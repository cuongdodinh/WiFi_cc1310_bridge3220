//26.08.17

#include <pthread.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"

//#include "uart_if.h"
#include "uart.h"

#include "cc1310_interface.h"
#include "global.h"
#include "appUtils.h"

sem_t rxDoneSem, UARTBufferBusy;


#define UART_TASK_STACK_SIZE    1024
#define UART_TASK_PRIORITY      2

#define UART_PACKET_MAX_SIZE    128
#define UART_READ_BUFFER_SIZE   1024
#define UART_WRITE_BUFFER_SIZE  1512

#define UART_PACKET_LEN_OFFSET        0
#define UART_PACKET_SEQ_NUM_OFFSET    1
#define UART_PACKET_CRC_OFFSET        2
#define UART_PACKET_PAYLOAD_OFFSET    4

#define UART_PACKET_MAX_PAYLOAD_SIZE    UART_PACKET_MAX_SIZE - UART_PACKET_PAYLOAD_OFFSET

#define UART_EVENT_ALL                               0xFFFFFFFF
#define UART_EVENT_RX_DONE         (_u32)(1 << 0)


#define WAIT_FOR_CONNECTION_REQUEST_TIMEOUT    100000   //in nanoseconds
#define ACK_TIMEOUT                            1000000  //in nanoseconds

static _u8 localBuffer [UART_PACKET_MAX_SIZE];
static _u8 readBuffer  [UART_READ_BUFFER_SIZE];
static _u8 writeBuffer [UART_WRITE_BUFFER_SIZE];
static _u8 connectionQueryByte = 0;
static short readLen = 0;
static _u8 readSeqNum = 0;
static _u8 writeSeqNum = 0;

_u16 readBufferLen = 0;
_u16 writeBufferLen = 0;

UART_Handle uart;

static void ReadCallback (UART_Handle handle, void *buf, size_t count);

//----------------------------------------------------------------------------------------------------------------------------------
void* UARTTask (void *pvParameters)
{
    sem_init (&rxDoneSem, 0, 0);
    sem_init (&UARTBufferBusy, 0, 0);
    sem_post (&UARTBufferBusy);

    UART_Params         uartParams;
    UART_Params_init(&uartParams);

    uartParams.writeDataMode    = UART_DATA_BINARY;
    uartParams.readDataMode     = UART_DATA_BINARY;
    uartParams.readReturnMode   = UART_RETURN_FULL;
    uartParams.readEcho         = UART_ECHO_OFF;
    uartParams.readMode         = UART_MODE_CALLBACK;
    uartParams.readCallback     = ReadCallback;

    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL)
        RebootMCU();

    struct timespec rxTimeout;
    rxTimeout.tv_sec = 0;

    UART_PRINT ("UARTTask started\n\r");

    while(1)
    {
        readLen = -1;
        UART_read (uart, &localBuffer, 1);

        rxTimeout.tv_nsec = WAIT_FOR_CONNECTION_REQUEST_TIMEOUT;

        if ((sem_timedwait (&rxDoneSem, &rxTimeout) == -1) || (readLen == 0))
        {
            if (readLen == -1)
            {
                UART_readCancel (uart);
                sem_wait (&rxDoneSem);
            }
            else
                BlinkLED (0);
        }
        else
        {
            char buf[256];
            int len = ToHexString (localBuffer, 1, buf);
            buf [len] = 0;

            UART_PRINT ("%s\n\r", buf);
            UART_PRINT ("%s\n\r", localBuffer);
        }


    }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void ReadCallback (UART_Handle handle, void *buf, size_t count)
{
    readLen = count;

    sem_post (&rxDoneSem);
}

