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
#include "socketClient.h"

static sem_t UARTBufferBusy;

static _u8 localBuffer [UART_PACKET_MAX_SIZE];
static _u8 readBuffer  [UART_PACKET_MAX_SIZE];
static _u8 writeBuffer [UART_WRITE_BUFFER_SIZE];
static _u8 readSeqNum = 0;
static _u8 writeSeqNum = 0;

_u16 readBufferLen = 0;
_u16 writeBufferLen = 0;
uint32_t lastWriteErrorTime = 0; //in ticks

UART_Handle uart;

static bool ReadPacket ();
static bool ReadHeader ();
static bool SendLocalBuffer (_u8 len);
static bool SendPacket ();
static void FlushUARTBuffer (int period);
static void UART_Connect ();
static void UART_Reconnect ();
static void ProcessPacket ();

//----------------------------------------------------------------------------------------------------------------------------------
void UARTTaskInit ()
{
    sem_init (&UARTBufferBusy, 0, 0);
    sem_post (&UARTBufferBusy);
}

//----------------------------------------------------------------------------------------------------------------------------------
void* UARTTask (void *pvParameters)
{

//    UART_AddToWriteBuffer ("Replay 1", sizeof ("Replay 1"));

    UART_Connect ();

    UART_PRINT ("UARTTask started \n\r");


    while(1)
    {
        Task_sleep (1);
//        UART_PRINT ("%s \n\r", StopwatchPrintStoredIntervals ());
        StopwatchRestart();

        GPIO_write(Board_LED0, Board_LED_OFF);
        GPIO_write(Board_LED1, Board_LED_OFF);
        StopwatchStoreInterval(0);

        if (ReadHeader() == false)
        {
            UART_Reconnect ();
            continue;
        }

        StopwatchStoreInterval(1);

        if (ReadPacket () == true)
        {

            _u8 len = localBuffer [UART_PACKET_LEN_OFFSET];

            if (len > UART_READ_BUFFER_SIZE)
            {
                UART_Reconnect ();

                LOG_ERROR (len);
                continue;
            }

            memcpy (&readBuffer, &localBuffer [UART_PACKET_PAYLOAD_OFFSET], len);
            readBufferLen = len;

/*            _u16 crc = GetCRC16 (&localBuffer [UART_PACKET_PAYLOAD_OFFSET + 2], localBuffer [UART_PACKET_LEN_OFFSET] - 2);

            if (memcmp (&crc, &localBuffer [UART_PACKET_PAYLOAD_OFFSET], 2) != 0)
            {
                UART_PRINT ("\n");
                LOG_ERROR (crc);
            }*/

            UART_write (uart, localBuffer, UART_PACKET_HEADER_SIZE);

//            if (readSeqNum % 7 == 0)
//                continue;

            if (writeBufferLen != 0)
            {
                if (SendPacket () == false)
                    UART_Reconnect ();
            }
            else
            {
                _u8 zero = 0;

                 UART_write (uart, &zero, 1);
            }

            ProcessPacket();

/*            _u8 buf[256];
            sprintf ((char*) buf, "   Replay qwertyuiopasdfghjklzxcvbnm  %d", s);
            int bufLen = strlen ((char*)buf);
            s++;

            crc = GetCRC16 (buf + 2, bufLen - 2);
            memcpy (buf, &crc, 2);

            UART_AddToWriteBuffer (buf, bufLen);*/
        }
        else
            UART_Reconnect ();

        StopwatchStoreInterval(13);


    }
}

//----------------------------------------------------------------------------------------------------------------------------------
static void ProcessPacket ()
{
   switch (readBuffer[0])
   {
    case UART_PACKET_TYPE_PACKET_FROM_NODE:
    case UART_PACKET_TYPE_BRIDGE_ERROR:
    case UART_PACKET_TYPE_PACKET_TO_NODE_ACK:

        SocketClientSendNodePacket(readBuffer, readBufferLen);
        break;

    case UART_PACKET_TYPE_ALIVE:
        break;

    default:
        LOG_ERROR(readBuffer [0]);

   }

}

//----------------------------------------------------------------------------------------------------------------------------------
static void UART_Connect ()
{
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
}

//----------------------------------------------------------------------------------------------------------------------------------
static void UART_Reconnect ()
{
    UART_PRINT ("UART_Reconnect\n\r");

    FlushUARTBuffer (UART_RECONNECT_PERIOD / 2);

    UART_close (uart);

    UART_Connect ();


    int rxCount = 0;

    unsigned int startTicks = Clock_getTicks();

    while (1)
    {
        UART_control (uart, UART_CMD_GETRXCOUNT, &rxCount);

        if (rxCount > 0)
            break;

        if (Clock_getTicks() - startTicks > UART_RECONNECT_PERIOD)
            break;
    }

    LOG_ASSERT (rxCount > 0);

}

//----------------------------------------------------------------------------------------------------------------------------------
static void FlushUARTBuffer (int period)
{
    unsigned int startTicks = Clock_getTicks();
    _u8 buf;

    int s = 0;
    for (s = 0; s < MAX_LOOP; s++)
    {
        UART_read (uart, &buf, 1);

        if (Clock_getTicks() - startTicks > period)
            break;
    }

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void UART_AddToWriteBuffer (_u8* data, _u8 len)
{
    if (len < 4)
        LOG_ERROR (len);

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
    localBuffer[UART_PACKET_LEN_OFFSET] = len;
    localBuffer[UART_PACKET_SEQ_NUM_OFFSET] = writeSeqNum;
    writeSeqNum++;

    _u16 crc = GetCRC16 (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], len);
    memcpy (&localBuffer[UART_PACKET_CRC_OFFSET], &crc, 2);

    UART_write (uart, localBuffer, len + UART_PACKET_PAYLOAD_OFFSET);

    _u8 ackPacket [UART_PACKET_HEADER_SIZE];

    int readLen = UART_read (uart, &ackPacket, sizeof (ackPacket));

    if (readLen != sizeof (ackPacket))
    {
        LOG_ERROR (readLen);
        return false;
    }
    if (memcmp (ackPacket, localBuffer, sizeof (ackPacket) - 1) != 0)
    {
        LOG_ERROR (ackPacket [1]);
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static bool SendPacket ()
{
    if (writeBufferLen == 0)
    {
        LOG_ERROR(0);
        return false;
    }

    GPIO_write(Board_LED0, Board_LED_ON);

    _u16 index = 0;


    sem_wait (&UARTBufferBusy);

    _u8 len = writeBuffer [index];
    index++;

    if (len > UART_PACKET_MAX_PAYLOAD_SIZE)
    {
        LOG_ERROR (len);
        return false;
    }

    if (len == 0)
    {
        LOG_ERROR (index);
        return false;
    }
    else if (index + len > writeBufferLen)
    {
        LOG_ERROR (index);
        return false;
    }

    memcpy (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], &writeBuffer [index], len);
    index += len;

    sem_post (&UARTBufferBusy);



    _u8 packetFullLen = len + UART_PACKET_HEADER_SIZE;

    UART_write (uart, &packetFullLen, 1);

    if (SendLocalBuffer (len) == false)
    {
        return false;
    }
    else
    {
        sem_wait (&UARTBufferBusy);

        int s;
        for (s = index; s < writeBufferLen; s++)
            writeBuffer [s - index] = writeBuffer[s];

        writeBufferLen -= index;

        sem_post (&UARTBufferBusy);
    }

    PrintBuffer ("SEND>", localBuffer, packetFullLen);


    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------
static bool SafeRead (_u8* readBuffer, _u8 len, int maxReadCount)
{
    int readLen = 0;
    int readCount = 0;
    int readResult = 0;

    while (readLen != len)
    {
        readResult = UART_read (uart, readBuffer + readLen, len - readLen);

        if (readResult < 0)
        {
            LOG_ERROR (readResult);
            return false;
        }

        readLen += readResult;

        if (readCount++ > maxReadCount)
        {
            LOG_ERROR2 (readLen, maxReadCount);

            if (readLen != 0)
            {
                PrintBuffer ("Safe read err>", localBuffer, readLen + 4);

            }
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------
static bool ReadHeader ()
{
    return SafeRead (localBuffer, UART_PACKET_HEADER_SIZE, UART_MAX_HEADER_READ_COUNT);
}

//----------------------------------------------------------------------------------------------------------------------------------
static bool ReadPacket ()
{
    GPIO_write(Board_LED1, Board_LED_ON);

    _u8 len = localBuffer [UART_PACKET_LEN_OFFSET];

    if (SafeRead (&localBuffer[UART_PACKET_PAYLOAD_OFFSET], len, UART_MAX_PACKET_READ_COUNT) == false)
        return false;

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
        int printLen = ToHexString (localBuffer, len + UART_PACKET_HEADER_SIZE, buf);
        buf [printLen] = 0;
        localBuffer [len + UART_PACKET_HEADER_SIZE] = 0;

        UART_PRINT ("crc error >%d   %d   %s    %s\r\n", len, crc, buf, &localBuffer[UART_PACKET_PAYLOAD_OFFSET]);
        return false;
    }

    if (localBuffer [UART_PACKET_PACKET_TYPE_OFFSET] != UART_PACKET_TYPE_ALIVE)
        PrintBuffer ("READ<", localBuffer, len + UART_PACKET_HEADER_SIZE);


    return true;
}


