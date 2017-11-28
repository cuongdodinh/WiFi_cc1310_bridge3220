//26.08.17

#include <stdio.h>
#include <stdlib.h>

#include <ti/sysbios/knl/Clock.h>

#include "global.h"
#include "simplelink.h"
//#include "network_if.h"
//#include "common.h"
#include "hw_types.h"
#include "prcm.h"
//#include "uart_if.h"

#include "appUtils.h"
#include "socketClient.h"
#include "socketProtocol.h"
#include "OTA_Update.h"
#include "cc1310_interface.h"


//#define SOCKET_SERVER_HOST_NAME       "192.168.2.228"
#define SOCKET_SERVER_HOST_NAME       "91.244.253.100"
//#define SOCKET_SERVER_HOST_NAME       "172.20.10.12"
#define SOCKET_SERVER_HOST_PORT       3000

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define MAX_WAIT_RECV_MS           10 * 1000

void SocketClientProcessPingResponse(_u16 packetLen);
void SocketClientProcessAuthResponse(_u16 packetLen);
void SocketClientLogErrorPacketType(_u16 packetLen);
void SocketClientProcessFile(_u16 packetLen);
void SocketClientProcessNodePacket(_u16 packetLen);

/*!
 *  \brief  Function pointer to the Packet handler
 */
typedef void (*fptr_PacketHandler)(_u16 packetLen);

const fptr_PacketHandler packetHandlers[] =
{
    SocketClientLogErrorPacketType,
    SocketClientProcessAuthResponse,  //PType_Auth
    SocketClientProcessPingResponse,  //PType_Ping
    SocketClientLogErrorPacketType,   //PType_SensorData
    SocketClientProcessFile,          //PType_File = 4,
    SocketClientLogErrorPacketType,   //PType_Log = 5
    SocketClientProcessNodePacket,    //PType_NodePacket = 6
    SocketClientLogErrorPacketType,   //
    SocketClientLogErrorPacketType,   //
    SocketClientLogErrorPacketType,   //
    SocketClientLogErrorPacketType,   //
    SocketClientLogErrorPacketType    //

};

static _u8 socketSendBuff [MAX_BUFF_SIZE];
static _u8 socketSendQueueBuff [SOCKET_SEND_QUEUE_BUFF_SIZE + SocketClient_AddToSendBuffer_OverlfowErrorPacketSize];
static short socketSendQueueBuffLen = 0;
static _u8 socketRecvBuff [MAX_BUFF_SIZE];
static _u8 *socketRecvBuffPacket = socketRecvBuff + HeaderOffsetsSecondEnd;
static _u8 g_nextGetPacketNum = 0;
static _u8 nextSendPacketNum = 0;
static int iSockID = -1;
_u16 g_ping = 0;

SlSockAddrIn_t  sAddr;
int             iAddrSize = sizeof(SlSockAddrIn_t);

unsigned long ulSecs;
unsigned short usMsec;

static sem_t socketSendQueueBuffBusy, socketSendPacketBusy;

bool SocketClientAuth();

//----------------------------------------------------------------------------------------------------------------------------------
void SocketClientInit ()
{
    sem_init (&socketSendQueueBuffBusy, 0, 0);
    sem_post (&socketSendQueueBuffBusy);

    sem_init (&socketSendPacketBusy, 0, 0);
    sem_post (&socketSendPacketBusy);
}

//----------------------------------------------------------------------------------------------------------------------------------
bool SocketClient_AddToSendBuffer (_u8* data, _u8 len)
{
    if (socketSendQueueBuffLen + len + 1 > SOCKET_SEND_QUEUE_BUFF_SIZE)
    {
        char errorText[] = SocketClient_AddToSendBuffer_OverlfowErrorPacketText;
        if ((len != SocketClient_AddToSendBuffer_OverlfowErrorPacketSize) || (memcmp (data + 4, errorText, len - 4) != 0)) //this is not overflow error packet
        {
            SocketClientSendLog(errorText, LogPart_ClientRuntime, LogType_Error);
            return false;
        }
    }

    if (socketSendQueueBuffLen + len + 1 > SOCKET_SEND_QUEUE_BUFF_SIZE + SocketClient_AddToSendBuffer_OverlfowErrorPacketSize)
        return false;

    sem_wait (&socketSendQueueBuffBusy);

    socketSendQueueBuff [socketSendQueueBuffLen] = len;
    socketSendQueueBuffLen++;

    memcpy (&socketSendQueueBuff [socketSendQueueBuffLen], data, len);
    socketSendQueueBuffLen += len;

    sem_post (&socketSendQueueBuffBusy);

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientProcessSend()
{
    if (iSockID == -1)
    {
        LOG_ERR();
        return false;
    }

    int loop;
    int len;

    for (loop = 0; loop < MAX_LOOP; loop++)
    {
        sem_wait (&socketSendQueueBuffBusy);

        if (socketSendQueueBuffLen == 0)
        {
            sem_post (&socketSendQueueBuffBusy);
            break;
        }

        len = socketSendQueueBuff[0];

        if (len + 1 > socketSendQueueBuffLen)
        {
            LOG_ERROR2 (len, socketSendQueueBuffLen);
            sem_post (&socketSendQueueBuffBusy);
            break;
        }

        memcpy (&socketSendBuff [HeaderOffsetsSecondEnd], &socketSendQueueBuff[1], len);

        sem_post (&socketSendQueueBuffBusy);

        if (SocketClientSendPacket (socketSendBuff, HeaderOffsetsSecondEnd + len) == true)
        {
            sem_wait (&socketSendQueueBuffBusy);

            ShiftByteArray (socketSendQueueBuff, socketSendQueueBuffLen, len + 1);
            socketSendQueueBuffLen -= len + 1;

            sem_post (&socketSendQueueBuffBusy);
        }
        else
        {
            LOG_ERR();
//            SocketClientReconnect ();
            return false;
        }


    }

    if (loop == MAX_LOOP)
    {
        LOG_ERR();
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientConnect ()
{
    //SOCKET------
     unsigned long   ulSocketServerIP;
    //    uint16_t serverTimeMS = 0;
     unsigned short usPort = SOCKET_SERVER_HOST_PORT;

     PRCMRTCInUseSet();
     PRCMRTCSet (0, 0);

     int lRetVal = Network_IF_GetHostIP((char*)SOCKET_SERVER_HOST_NAME, &ulSocketServerIP);
     if (lRetVal < 0)
     {
         g_appState = SERVER_CONNECTION_ERROR;
         UART_PRINT("SOCKET DNS lookup failed. %d \n\r",lRetVal);
         return false;
     }

     //filling the TCP server socket address
     sAddr.sin_family = SL_AF_INET;
     sAddr.sin_port = sl_Htons((unsigned short)usPort);
     sAddr.sin_addr.s_addr = sl_Htonl((unsigned int)ulSocketServerIP);

     // creating a TCP socket
     iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
     if( iSockID < 0 )
     {
         g_appState = SERVER_CONNECTION_ERROR;
         UART_PRINT("SOCKET_ID ERROR %d \n\r",iSockID);
         return false;
     }

     UART_PRINT("SOCKET_ID %d \n\r",iSockID);

     // connecting to TCP server
     lRetVal = sl_Connect(iSockID, ( SlSockAddr_t *)&sAddr, iAddrSize);
     if( lRetVal < 0 )
     {
         g_appState = SERVER_CONNECTION_ERROR;

         SocketClientDisconnect ();
         UART_PRINT("SOCKET_CONNECT_ERROR %d \n\r",lRetVal);
         return false;
     }

     g_appState = DEVICE_CONNECTED_SERVER;

     nextSendPacketNum = 0;
     g_nextGetPacketNum = 0;
     if (SocketClientAuth() == false)
     {
         SocketClientDisconnect ();
         g_appState = SERVER_CONNECTION_ERROR;
         return false;
     }

     g_appState = DEVICE_CONNECTED_SERVER_AUTH;

     char buff[100];
     sprintf (buff, "Connected. Ver: %d", APPLICATION_VERSION);
     SocketClientSendLog(buff, LogPart_ClientRuntime, LogType_None);

     return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientDisconnect ()
{
    sl_Close(iSockID);
    iSockID = -1;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientReconnect ()
{
    SocketClientDisconnect ();

    SocketClientConnect ();

    SocketClientSendLog("Reconnected", LogPart_ClientRuntime, LogType_None);

}

//--------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendPacket (_u8 *socketSendBuff, int packetLen)
{
    if (sem_trywait (&socketSendPacketBusy) != 0)
    {
        LOG_ERR();
        sem_wait (&socketSendPacketBusy);
    }

    _u16 packetCRC;

    _u16 clearPacketLen = packetLen - HeaderOffsetsFirstEnd;

    memcpy (&socketSendBuff [0], &clearPacketLen, 2);

    socketSendBuff [HeaderOffsetsPacketNum] = nextSendPacketNum;
    nextSendPacketNum++;

    memcpy (&socketSendBuff [HeaderOffsetsMac], macAddressVal, 6);


    packetCRC = GetCRC16 (&socketSendBuff[HeaderOffsetsFirstEnd], clearPacketLen);
    memcpy (&socketSendBuff [HeaderOffsetsCheckSum], &packetCRC, 2);

    char printBuff[200];

    _u8 len = ToHexString ((_u8*)&socketSendBuff [0], MIN (packetLen, sizeof (printBuff) / 2), printBuff);
//    UART_PRINT("Packet: %.*s\n\r", len, printBuff);

    int lRetVal = sl_Send(iSockID, socketSendBuff, packetLen, 0 );
    if( lRetVal < 0 )
    {
        LOG_ERROR2 (lRetVal, packetLen);
//        SocketClientReconnect ();
//        RebootMCU();

        sem_post (&socketSendPacketBusy);
        return false;
    }

    sem_post (&socketSendPacketBusy);
    return true;
}

_u16 bytesReceived = 0;

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int SocketClientProcessRecv ()
{
    int recvLen = 0;
    int maxRecvLen = 0;

    if (bytesReceived < HeaderOffsetsFirstEnd)
        maxRecvLen = HeaderOffsetsFirstEnd - bytesReceived; //ждем сначала заголовок с размером пакета
    else
    {
        _u16 packetLen = *((_u16*) (socketRecvBuff));

        maxRecvLen = packetLen + HeaderOffsetsFirstEnd - bytesReceived;

        if (maxRecvLen + bytesReceived > sizeof (socketRecvBuff))
        {
            LOG_ERROR (packetLen);
            return -1;
        }
    }

    recvLen = sl_Recv(iSockID, socketRecvBuff + bytesReceived, maxRecvLen, SL_MSG_DONTWAIT);

//    UART_PRINT ("recvLen: %d   %d    %d\n\r", recvLen, maxRecvLen, bytesReceived);

    if (recvLen > 0)
    {
        bytesReceived += recvLen;

        if (bytesReceived > HeaderOffsetsSecondEnd) //пришло данных не меньше, чем минимально возможная длина пакета
        {
            _u16 packetLen = *((_u16*) (socketRecvBuff));

            if (packetLen > bytesReceived - HeaderOffsetsFirstEnd) //пришел не весь пакет, ждем продолжения
                return 0;

            else if (packetLen < bytesReceived - HeaderOffsetsFirstEnd) //пришло данных больше чем нужно - ошибка
            {
                LOG_ERROR (bytesReceived);
                return -2;
            }
            //пакет пришел целиком и ровно в правильном размере

            if (*((_u8*) (socketRecvBuff + HeaderOffsetsPacketNum)) != g_nextGetPacketNum) //очередность пакетов нарушена - ошибка
            {
                LOG_ERROR (g_nextGetPacketNum);
                return -5;
            }

            _u16 packetCRC = *((_u16*) (socketRecvBuff + HeaderOffsetsCheckSum));

            _u16 crc = GetCRC16 (&(socketRecvBuff [HeaderOffsetsFirstEnd]), packetLen);
            if (crc != packetCRC)
            {
                UART_PRINT ("CRC Error: %d   %d\n", crc, packetCRC);
                LOG_ERROR (crc);

                char printBuff[200];
                _u8 len = ToHexString (socketRecvBuff, MIN (bytesReceived, sizeof (printBuff) / 2), printBuff);
                UART_PRINT("GET err: %d   %.*s\n\r", bytesReceived, len, printBuff);

                return -3;
            }

            if (memcmp (socketRecvBuff + HeaderOffsetsMac, macAddressVal, 6) != 0) //пришел пакет с неправильным mac-адресом получателя
            {
                char buff[150];
                sprintf (buff, "SocketClientProcessRecv. pMAC: ");

                char macBuff [13];
                macBuff [12] = 0;
                ToHexString (socketRecvBuff + HeaderOffsetsMac, 6, macBuff);
                strcat (buff, (char*) macBuff);

                strcat (buff, ", cMAC: ");

                ToHexString (macAddressVal, 6, macBuff);
                strcat (buff, (char*) macBuff);
                strcat (buff, "\n\r");

                LogError (buff);

                return -4;
            }



            //все проверки пакета прошли успешно, обрабатываем пакет

            g_nextGetPacketNum++;

            char printBuff[200];
            _u8 len = ToHexString (socketRecvBuff, MIN (bytesReceived, sizeof (printBuff) / 2), printBuff);
            UART_PRINT("GET: %d   %.*s\n\r", bytesReceived, len, printBuff);

            if (socketRecvBuffPacket [0] >= sizeof (packetHandlers) / sizeof (packetHandlers[0]))
            {
                LOG_ERROR (socketRecvBuffPacket [0] );
                return -6;
            }

            packetHandlers [socketRecvBuffPacket [0]] (packetLen - HeaderOffsetsSecondEnd + HeaderOffsetsFirstEnd);
            bytesReceived = 0;

            return socketRecvBuffPacket [0];
        }
    }
    else if (recvLen != -11)
    {
        LOG_ERROR (recvLen);
        return -5;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientAuth()
{
    socketSendBuff [HeaderOffsetsSecondEnd] = PType_Auth;

    *((_u16*) (socketSendBuff + HeaderOffsetsSecondEnd + 1)) = APPLICATION_VERSION;

    if (SocketClientSendPacket (socketSendBuff, HeaderOffsetsSecondEnd + 3) == false)
    {
        LOG_ERR();
        return false;
    }

    int s;

    for (s = 0; s < MAX_WAIT_RECV_MS / 10; s++)
    {
        int retVal = SocketClientProcessRecv();

        if (retVal < 0)
            return false;
        else if (retVal > 0)
        {
            if (retVal == PType_Auth)
                return true;
            else
                return false;
        }

        Task_sleep (10000 / Clock_tickPeriod);
    }

    return false;
}

void SocketClientProcessAuthResponse(_u16 packetLen)
{
    if (packetLen != 1)
        LOG_ERROR (packetLen);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientPing()
{
    // sending Ping packet
    PRCMRTCGet(&ulSecs, &usMsec);

    _u32 clientTimeMS = ulSecs * 1000 + usMsec;

    _u8 packetBuffer [7];

    packetBuffer [0] = PType_Ping;
    memcpy (&packetBuffer [1], &clientTimeMS, 4);
    memcpy (&packetBuffer [5], &g_ping, 2);

    return SocketClient_AddToSendBuffer (packetBuffer, sizeof (packetBuffer));
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientProcessPingResponse(_u16 packetLen)
{
    if (packetLen != 5)
    {
        LOG_ERROR (packetLen);
        return;
    }

    _u32 lastClientTimeMS = *((_u32*) (socketRecvBuffPacket + 1));

    PRCMRTCGet(&ulSecs, &usMsec);

    g_ping = ulSecs * 1000 + usMsec - lastClientTimeMS;
//    UART_PRINT("ping: %d \n\r", g_ping);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
_u8 SocketBufferGetString (_u16 pos, char *str)
{
    _u8 len = socketRecvBuffPacket [pos];

    memcpy (str, socketRecvBuffPacket + pos + 1, len);

    str [len] = 0;

    return len;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
_u16 SocketBufferGetBytes (_u16 pos, _u8 *buff)
{
    _u16 len = *((_u16*) (socketRecvBuffPacket + pos));

    memcpy (buff, socketRecvBuffPacket + pos + 2, len);

    return len;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientProcessNodePacket (_u16 packetLen)
{
    _u8 packet [packetLen];

    memcpy (&packet, socketRecvBuffPacket, packetLen);

    packet [0] = UART_PACKET_TYPE_PACKET_TO_NODE; //change PacketType byte to UART_PACKET_TYPE_PACKET_TO_NODE

    UART_AddToWriteBuffer (packet, packetLen);

//    UART_PRINT ("SocketNodePacket: %d\n\r", packetLen);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientProcessFile(_u16 packetLen)
{
    if (packetLen < 2)
    {
        LOG_ERROR (packetLen);
        return;
    }

    _u8 packetSubType = *((_u8*) (socketRecvBuffPacket + 1));

    switch (packetSubType)
    {
        case PSubType_FileStart:
        {
            char fileName [257];

            _u16 pos = 2;
            pos += SocketBufferGetString (pos, fileName) + 1;

            _u32 fileMaxSize = *((_u32*) (socketRecvBuffPacket + pos));

            OTA_Start (fileName, fileMaxSize);

            break;
        }
        case PSubType_FileContinue:
        {
            _u16 partSize = *((_u16*) (socketRecvBuffPacket + 2));
            OTA_Continue (socketRecvBuffPacket + 4, partSize);

            break;
        }

        case PSubType_FileEnd:
        {
            char certName [257];
            _u8 signBuff [512];

            _u16 pos = 2;
            _u32 fileSize = *((_u32*) (socketRecvBuffPacket + pos));
            pos +=4;

            _u64 fileCheckSum = *((_u64*) (socketRecvBuffPacket + pos));
            pos +=8;

            pos += SocketBufferGetString (pos, certName) + 1;

            _u16 signLen = SocketBufferGetBytes (pos, signBuff);

            OTA_End (fileSize, fileCheckSum, certName, signBuff, signLen);

            break;
        }
    }

//    Platform_Sleep(1000);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendLog(char* text, _u8 logPart, _u8 logType)
{
    _u8 len = strlen (text);
    _u8 packetBuffer [4 + len];

    packetBuffer [0] = PType_Log;
    packetBuffer [1] = logPart;
    packetBuffer [2] = logType;

    packetBuffer [3] = len;

    memcpy (&packetBuffer[4], text, len);

    return SocketClient_AddToSendBuffer (packetBuffer, sizeof (packetBuffer));
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendNodePacket(_u8* data, _u8 len)
{
    _u8 packetBuffer [1 + len];

    packetBuffer [0] = PType_NodePacket;

    memcpy (&packetBuffer[1], data, len);

    return SocketClient_AddToSendBuffer (packetBuffer, sizeof (packetBuffer));
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendSensorData (char* sensorMac, float sensorValue1, float sensorValue2)
{
    _u8 packetBuffer [17];

    packetBuffer [0] = PType_SensorData;

    memcpy (&packetBuffer [1], sensorMac, 8);

    memcpy (&packetBuffer [9], &sensorValue1, 4);
    memcpy (&packetBuffer [13], &sensorValue2, 4);

    return SocketClient_AddToSendBuffer (packetBuffer, sizeof (packetBuffer));
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientLogErrorPacketType (_u16 packetLen)
{
    LOG_ERROR (socketRecvBuffPacket [0]);
}

//*****************************************************************************
//
//!    prints the formatted string on to the console
//!
//! \param format is a pointer to the character string specifying the format in
//!           the following arguments need to be interpreted.
//! \param [variable number of] arguments according to the format in the first
//!         parameters
//! This function
//!        1. prints the formatted error statement.
//!
//! \return count of characters printed
//
//*****************************************************************************
void LogError (const char *pcFormat, ...)
{
  int iRet = 0;

  char *pcBuff, *pcTemp;
  int iSize = 256;

  va_list list;
  pcBuff = (char*)malloc(iSize);
  if(pcBuff == NULL)
  {
      return;
  }
  while(1)
  {
      va_start(list,pcFormat);
      iRet = vsnprintf(pcBuff,iSize,pcFormat,list);
      va_end(list);
      if(iRet > -1 && iRet < iSize)
      {
          break;
      }
      else
      {
          iSize*=2;
          if((pcTemp=realloc(pcBuff,iSize))==NULL)
          {
              Message("Could not reallocate memory\n\r");
              iRet = -1;
              break;
          }
          else
          {
              pcBuff=pcTemp;
          }

      }
  }
  Message(pcBuff);
  SocketClientSendLog(pcBuff, LogPart_ClientRuntime, LogType_Error);
  free(pcBuff);
}

//*****************************************************************************
void Log (const char *pcFormat, ...)
{
  int iRet = 0;

  char *pcBuff, *pcTemp;
  int iSize = 256;

  va_list list;
  pcBuff = (char*)malloc(iSize);
  if(pcBuff == NULL)
  {
      return;
  }
  while(1)
  {
      va_start(list,pcFormat);
      iRet = vsnprintf(pcBuff,iSize,pcFormat,list);
      va_end(list);
      if(iRet > -1 && iRet < iSize)
      {
          break;
      }
      else
      {
          iSize*=2;
          if((pcTemp=realloc(pcBuff,iSize))==NULL)
          {
              Message("Could not reallocate memory\n\r");
              iRet = -1;
              break;
          }
          else
          {
              pcBuff=pcTemp;
          }

      }
  }
  Message(pcBuff);
  SocketClientSendLog(pcBuff, LogPart_ClientRuntime, LogType_None);
  free(pcBuff);
}
