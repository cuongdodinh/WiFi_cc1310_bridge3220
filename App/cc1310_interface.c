//26.08.17

#include <pthread.h>
#include <ti/sysbios/knl/Clock.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"

//#include "uart_if.h"
#include "uart.h"

#include "cc1310_interface.h"
#include "global.h"
#include "appUtils.h"

sem_t UARTBufferBusy;


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


#define RESPONCE_TIMEOUT                        507 * 1000 / Clock_tickPeriod //in uSeconds
#define WRITE_TIMEOUT_AFTER_ERROR              1000 * 1000 / Clock_tickPeriod //in uSeconds

static _u8 localBuffer [UART_PACKET_MAX_SIZE];
static _u8 readBuffer  [UART_READ_BUFFER_SIZE];
static _u8 writeBuffer [UART_WRITE_BUFFER_SIZE];
static _u8 connectionQueryByte = 0;
static _u8 readSeqNum = 0;
static _u8 writeSeqNum = 0;

_u16 readBufferLen = 0;
_u16 writeBufferLen = 0;
uint32_t lastWriteErrorTime = 0; //in ticks

UART_Handle uart;

static bool ReadToLocalBuffer (_u8 connectionQueryByte);
static bool SendLocalBuffer (_u8 len);
static void SendAllWriteBuffer ();

//----------------------------------------------------------------------------------------------------------------------------------
void* UARTTask (void *pvParameters)
{
    sem_init (&UARTBufferBusy, 0, 0);
    sem_post (&UARTBufferBusy);


//    UART_AddToWriteBuffer ("Replay 1", sizeof ("Replay 1"));

    UART_Params         uartParams;
    UART_Params_init(&uartParams);

    uartParams.writeDataMode    = UART_DATA_BINARY;
    uartParams.readDataMode     = UART_DATA_BINARY;
    uartParams.readReturnMode   = UART_RETURN_FULL;
    uartParams.readEcho         = UART_ECHO_OFF;
    uartParams.readTimeout      = RESPONCE_TIMEOUT;


    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL)
        RebootMCU();

    UART_PRINT ("UARTTask started \n\r");

    while(1)
    {
//        UART_PRINT ("%s \n\r", StopwatchPrintStoredIntervals ());
        StopwatchRestart();

        GPIO_write(Board_LED0, Board_LED_OFF);
        GPIO_write(Board_LED1, Board_LED_OFF);
        StopwatchStoreInterval(0);

        int  rxCount;
        rxCount = UART_read (uart, &connectionQueryByte, 1);

        if (rxCount < 0)
        {
            LOG_ERROR (rxCount);
            continue;
        }

        StopwatchStoreInterval(1);

        if (rxCount > 0)
        {
            StopwatchStoreInterval(2);

            if (connectionQueryByte & 0x80 != 0x80) //first bit must be 1 for master
            {
                LOG_ERROR (connectionQueryByte);
                continue;
            }
            _u8 len = 0x7F & connectionQueryByte;

            if (readBufferLen + len + 1 > UART_READ_BUFFER_SIZE)
                continue;

            if (ReadToLocalBuffer (len) == true)
            {
                readBuffer [readBufferLen] = len;
               // readBufferLen++;

                memcpy (&readBuffer [readBufferLen], &localBuffer [UART_PACKET_PAYLOAD_OFFSET], len);
              //  readBufferLen += len;

                //if (localBuffer [UART_PACKET_PAYLOAD_OFFSET] == 'T')
                {
                    int s;
                    _u8 buf[256];

                    for (s = 0; s < 1; s++)
                    {
                        //sprintf (buf, "Replay 12345678901234567890123456789012345678901234567890123456789012345678901234567890 %d", s);

                        UART_AddToWriteBuffer ("Replay 12345678901234567890123456789012345678901234567890123456789012345678901234567890", strlen ("Replay 12345678901234567890123456789012345678901234567890123456789012345678901234567890"));
                     }

                }
            }

            StopwatchStoreInterval(6);
//            UART_PRINT ("%s \n\r", StopwatchPrintStoredIntervals ());

            //char buf[256];
            //int printLen = ToHexString (&connectionQueryByte, 1, buf);
            //buf [printLen] = 0;

//            UART_PRINT ("-------------%s\n\r", buf);

        }

        if (rxCount == 0)
        {
            StopwatchStoreInterval(7);

            if ((Clock_getTicks() - lastWriteErrorTime) < WRITE_TIMEOUT_AFTER_ERROR)
                continue;

            SendAllWriteBuffer ();
            StopwatchStoreInterval(12);
        }
        StopwatchStoreInterval(13);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void UART_AddToWriteBuffer (_u8* data, _u8 len)
{
    if (len > UART_PACKET_MAX_PAYLOAD_SIZE)
    {
        LOG_ERROR (len);
        return;
    }
    if (writeBufferLen + len > UART_WRITE_BUFFER_SIZE)
    {
 //       LOG_ERROR2 (writeBufferLen, len);
        return;
    }


    sem_wait (&UARTBufferBusy);

    writeBuffer [writeBufferLen] = len;
    writeBufferLen++;

    memcpy (&writeBuffer [writeBufferLen], data, len);
    writeBufferLen += len;

    sem_post (&UARTBufferBusy);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static bool SendLocalBuffer (_u8 len)
{
    _u8 connectionQueryByte = 0x80 | len; //first bit must be 1 for master

    StopwatchStoreInterval(8);

    UART_write (uart, &connectionQueryByte, 1);

    StopwatchStoreInterval(9);

    int readLen = 0;
    int s;
    for (s = 0; s < 4; s++)
    {
        readLen = UART_read (uart, &connectionQueryByte, 1);

        if (readLen != 0)
            break;
    }


    StopwatchStoreInterval(10);

    if (readLen < 0)
    {
        LOG_ERROR (readLen);
        return false;
    }
    if (readLen == 0)
    {
        LOG_ERROR (0);
        StopwatchStoreInterval(11);
        return false;
    }

    if (connectionQueryByte != len) //first bit must be 0 for slave
    {
        LOG_ERROR (connectionQueryByte);
        return false;
    }

    localBuffer[UART_PACKET_LEN_OFFSET] = len;
    localBuffer[UART_PACKET_SEQ_NUM_OFFSET] = writeSeqNum;
    writeSeqNum++;

    _u16 crc = GetCRC16 (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], len);
    memcpy (&localBuffer[UART_PACKET_CRC_OFFSET], &crc, 2);

    UART_write (uart, localBuffer, len + UART_PACKET_PAYLOAD_OFFSET);

    _u8 ackQueryByte;

    for (s = 0; s < 2; s++)
    {
        readLen = UART_read (uart, &ackQueryByte, 1);

        if (readLen != 0)
            break;
    }

    if (readLen < 0)
    {
        LOG_ERROR (readLen);
        return false;
    }
    if (readLen == 0)
    {
        LOG_ERROR (0);
        return false;
    }

    if (ackQueryByte != ((localBuffer[UART_PACKET_CRC_OFFSET] + localBuffer[UART_PACKET_CRC_OFFSET + 1]) & 0x7F)) //first bit must be 0 for slave
    {
        LOG_ERROR (ackQueryByte);
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void SendAllWriteBuffer ()
{
    _u16 index = 0;

     while (index < writeBufferLen)
     {
         sem_wait (&UARTBufferBusy);

         bool packetError = false;

         _u8 len = writeBuffer [index];
         index++;

         if (len > UART_PACKET_MAX_PAYLOAD_SIZE)
         {
             LOG_ERROR (len);
             index += len;
             packetError = true;
         }
         else if (len == 0)
         {
             LOG_ERROR (index);
             packetError = true;
         }
         else if (index + len > writeBufferLen)
         {
             LOG_ERROR (index);
             index = writeBufferLen;
             packetError = true;
         }

         if (packetError == false)
         {
             memcpy (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], &writeBuffer [index], len);

             index += len;
         }

         if (index == writeBufferLen) //last packet was got, reset writeBuffer in thread-safe section
         {
             writeBufferLen = 0;
             index = 0;
         }

         sem_post (&UARTBufferBusy);

         if (packetError == false)
         {
             GPIO_write(Board_LED0, Board_LED_ON);

             if (SendLocalBuffer (len) == false)
             {
                 if (index == 0)
                 {
                     UART_AddToWriteBuffer (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], len); //will try to send packet next time
                 }
                 else
                 {
                     index -= len + 1;

                     int s;
                     for (s = index; s < writeBufferLen; s++)
                         writeBuffer [s - index] = writeBuffer[s];

                     writeBufferLen -= index;
                 }
                 break;
             }
         }
     }

}

//----------------------------------------------------------------------------------------------------------------------------------
static bool ReadToLocalBuffer (_u8 len)
{
    StopwatchStoreInterval(3);

    UART_write (uart, &len, 1); //first bit must be 0 for slave

    StopwatchStoreInterval(4);


    GPIO_write(Board_LED1, Board_LED_ON);


    int readLen = 0;
    int s;
    for (s = 0; s < 10; s++)
    {
        readLen += UART_read (uart, localBuffer + readLen, len + UART_PACKET_PAYLOAD_OFFSET - readLen);

        if (readLen == len + UART_PACKET_PAYLOAD_OFFSET)
            break;
    }

    StopwatchStoreInterval(5);

    if (readLen < 0)
    {
        LOG_ERROR (readLen);
        return false;
    }
    if (readLen == 0)
    {
        LOG_ERROR (readLen);
        return false;
    }
    if (readLen != len + UART_PACKET_PAYLOAD_OFFSET)
    {
        LOG_ERROR2 (readLen, len + UART_PACKET_PAYLOAD_OFFSET);
        char buf[256];
        int printLen = ToHexString (localBuffer, readLen, buf);
        buf [printLen] = 0;
        localBuffer [len + UART_PACKET_PAYLOAD_OFFSET] = 0;

        UART_PRINT ("Error >%d     %s\n\r", len, buf);
        UART_PRINT ("Error  %s\n\r",  &localBuffer[UART_PACKET_PAYLOAD_OFFSET]);
        return false;
    }
    if (len != localBuffer [UART_PACKET_LEN_OFFSET])
    {
        LOG_ERROR2 (len, localBuffer [UART_PACKET_LEN_OFFSET]);
        return false;
    }
    if (readSeqNum != localBuffer [UART_PACKET_SEQ_NUM_OFFSET])
    {
        LOG_ERROR2 (readSeqNum, localBuffer [UART_PACKET_SEQ_NUM_OFFSET]);
        readSeqNum = localBuffer [UART_PACKET_SEQ_NUM_OFFSET];
    }
    readSeqNum++;

    _u16 crc = *((_u16*) (localBuffer + UART_PACKET_CRC_OFFSET));
    if (crc != GetCRC16 (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], len))
    {
        LOG_ERROR (crc);

        char buf[256];
        int printLen = ToHexString (localBuffer, readLen, buf);
        buf [printLen] = 0;
        localBuffer [len + UART_PACKET_PAYLOAD_OFFSET] = 0;

        UART_PRINT ("crc error >%d   %d   %s    %s\r\n", len, crc, buf, &localBuffer[UART_PACKET_PAYLOAD_OFFSET]);
        return false;
    }

//    if (readSeqNum % 10 == 0)
//        return;

    _u8 ack = ((localBuffer[UART_PACKET_CRC_OFFSET] + localBuffer[UART_PACKET_CRC_OFFSET + 1]) & 0x7F);

    UART_write (uart, &ack, 1); //first bit must be 0 for slave

    char buf[256];
    int printLen = ToHexString (localBuffer, readLen, buf);
    buf [printLen] = 0;
    localBuffer [len + UART_PACKET_PAYLOAD_OFFSET] = 0;

    UART_PRINT (">%d   %d   %s    %s\r", len, crc, buf, &localBuffer[UART_PACKET_PAYLOAD_OFFSET]);
//    Task_sleep (100);

    return true;
}


