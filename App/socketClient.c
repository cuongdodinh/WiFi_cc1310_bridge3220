//26.08.17

#include <stdio.h>
#include <stdlib.h>
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


//#define SOCKET_SERVER_HOST_NAME       "192.168.2.228"
#define SOCKET_SERVER_HOST_NAME       "91.244.253.100"
//#define SOCKET_SERVER_HOST_NAME       "172.20.10.12"
#define SOCKET_SERVER_HOST_PORT       3000

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

_u8 socketSendBuff [MAX_BUFF_SIZE+1];
_u8 socketRecvBuff [MAX_BUFF_SIZE+1];
_u8 *socketRecvBuffPacket = socketRecvBuff + HeaderOffsetsSecondEnd;
_u8 nextGetPacketNum = 0;
_u8 nextSendPacketNum = 0;
int iSockID;
_u16 ping = 0;

SlSockAddrIn_t  sAddr;
int             iAddrSize = sizeof(SlSockAddrIn_t);

unsigned long ulSecs;
unsigned short usMsec;

bool SocketClientAuth();

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

         sl_Close(iSockID);
         UART_PRINT("SOCKET_CONNECT_ERROR %d \n\r",lRetVal);
         return false;
     }

     g_appState = DEVICE_CONNECTED_SERVER;

     nextSendPacketNum = 0;
     nextGetPacketNum = 0;
     if (SocketClientAuth() == false)
     {
         SocketClientDisconnect ();
         g_appState = SERVER_CONNECTION_ERROR;
         return false;
     }

     g_appState = DEVICE_CONNECTED_SERVER_AUTH;

     char buff[100];
     sprintf (buff, "Connected. SocketID: %d", iSockID);
     SocketClientSendLog(buff, LogPart_ClientRuntime, LogType_None);

     return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientDisconnect ()
{
    sl_Close(iSockID);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SocketClientReconnect ()
{
    sl_Close(iSockID);

    SocketClientConnect ();

    SocketClientSendLog("Reconnected", LogPart_ClientRuntime, LogType_None);

}

//--------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendPacket (_u8 *socketSendBuff, int packetLen)
{
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
        UART_PRINT("SendPacket(): sending packet_ERROR %d \n\r",lRetVal);
//        SocketClientReconnect ();
//        RebootMCU();

        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int GetPacket ()
{
    int recvLen = 0;
    int s;
    _u16 bytesReceived = 0;
    bool recvSuccess = false;

    for (s = 0; s < MAX_WAIT_RECV_MS; s++)
    {
        if (bytesReceived >= sizeof (socketRecvBuff))
        {
            LOG_ERROR (bytesReceived);
            recvSuccess = false;
            break;
        }

        recvLen = sl_Recv(iSockID, socketRecvBuff + bytesReceived, sizeof (socketRecvBuff) - bytesReceived, SL_MSG_DONTWAIT);
        if (recvLen > 0)
        {
            bytesReceived += recvLen;

            if (bytesReceived > HeaderOffsetsSecondEnd) //пришло данных не меньше, чем минимально возможная длина пакета
            {
                _u16 packetLen = *((_u16*) (socketRecvBuff));

                if (packetLen > bytesReceived - HeaderOffsetsFirstEnd) //пришел не весь пакет, ждем продолжения
                    continue;
                else if (packetLen < bytesReceived - HeaderOffsetsFirstEnd) //пришло данных больше чем нужно - ошибка
                {
                    LOG_ERROR (bytesReceived);
                    recvSuccess = false;
                    break;
                }
                //пакет пришел целиком и ровно в правильном размере

                _u16 packetCRC = *((_u16*) (socketRecvBuff + HeaderOffsetsCheckSum));

                _u16 crc = GetCRC16 (&(socketRecvBuff [HeaderOffsetsFirstEnd]), packetLen);
                if (crc != packetCRC)
                {
                    UART_PRINT ("CRC Error: %d   %d\n", crc, packetCRC);
                    LOG_ERROR (crc);
                    recvSuccess = false;
                    break;
                }

                if (memcmp (socketRecvBuff + HeaderOffsetsMac, macAddressVal, 6) != 0)
                {
                    char buff[150];
                    sprintf (buff, "GetPacket (memcmp (g_recvBuff + HeaderOffsetsMac, macAddressVal, 6) != 0).pMAC: ");

                    char macBuff [13];
                    macBuff [12] = 0;
                    ToHexString (socketRecvBuff + HeaderOffsetsMac, 6, macBuff);
                    strcat (buff, (char*) macBuff);

                    strcat (buff, ", cMAC: ");

                    ToHexString (macAddressVal, 6, macBuff);
                    strcat (buff, (char*) macBuff);
                    strcat (buff, "\n\r");

                    LogError (buff);

                    recvSuccess = false;
                    break;
                }

                if (*((_u8*) (socketRecvBuff + HeaderOffsetsPacketNum)) != nextGetPacketNum) //очередность пакетов нарушена - ошибка
                {
                    LOG_ERROR (nextGetPacketNum);
                    recvSuccess = false;
                    break;
                }

                //все проверки пакета прошли успешно

                recvSuccess = true;
                nextGetPacketNum++;
                break;
            }
        }


        Platform_Sleep(1);
    }

    if (s < MAX_WAIT_RECV_MS)
    {
        char printBuff[200];
        _u8 len = ToHexString (socketRecvBuff, MIN (bytesReceived, sizeof (printBuff) / 2), printBuff);
        UART_PRINT("GET: %d   %.*s\n\r", bytesReceived, len, printBuff);
    }
    else
        LOG_ERROR (recvLen);

    if (recvSuccess == false)
        g_appState = SERVER_CONNECTION_ERROR;

    return bytesReceived - HeaderOffsetsSecondEnd;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientAuth()
{
    socketSendBuff [HeaderOffsetsSecondEnd] = PType_Auth;

    if (SocketClientSendPacket (socketSendBuff, HeaderOffsetsSecondEnd + 1) == false)
    {
        // error
        UART_PRINT("SocketClientAuth sending packet_ERROR\n\r");
//        SocketClientReconnect ();
    }

    int packetLen = GetPacket ();

    if (packetLen > 0)
        if (socketRecvBuffPacket [0] == PType_Auth)
            return true;

    return false;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientPing()
{
    // sending Ping packet
    PRCMRTCGet(&ulSecs, &usMsec);

    _u32 clientTimeMS = ulSecs * 1000 + usMsec;

    socketSendBuff [HeaderOffsetsSecondEnd] = PType_Ping;
    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 1], &clientTimeMS, 4);
    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 5], &ping, 2);

    if (SocketClientSendPacket (socketSendBuff, HeaderOffsetsSecondEnd + 7) == false)
    {
        // error
        UART_PRINT("SocketClientSendPing sending packet_ERROR\n\r");
        SocketClientReconnect ();
        return false;
    }

    int packetLen = GetPacket();

    if (packetLen > 0)
    {
        if (socketRecvBuffPacket [0] != PType_Ping)
        {
           LOG_ERROR (socketRecvBuffPacket [0]);
           return false;
        }

        _u32 lastClientTimeMS = *((_u32*) (socketRecvBuffPacket + 1));

        if (lastClientTimeMS != clientTimeMS)
        {
            LOG_ERROR (clientTimeMS);
            return false;
        }

        PRCMRTCGet(&ulSecs, &usMsec);

        ping = ulSecs * 1000 + usMsec - lastClientTimeMS;
        UART_PRINT("ping: %d \n\r", ping);

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendLog(char* text, _u8 logPart, _u8 logType)
{
    socketSendBuff [HeaderOffsetsSecondEnd] = PType_Log;
    socketSendBuff [HeaderOffsetsSecondEnd + 1] = logPart;
    socketSendBuff [HeaderOffsetsSecondEnd + 2] = logType;

    socketSendBuff [HeaderOffsetsSecondEnd + 3] = strlen (text);

    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 4], text, strlen (text));

    if (SocketClientSendPacket (socketSendBuff, HeaderOffsetsSecondEnd + 4 + strlen (text)) == false)
    {
        // error
        UART_PRINT("SocketClientSendLog sending packet_ERROR\n\r");
        return false;
 //       SocketClientReconnect ();
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool SocketClientSendSensorData (char* sensorMac, float sensorValue1, float sensorValue2)
{
    socketSendBuff [HeaderOffsetsSecondEnd] = PType_SensorData;

    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 1], sensorMac, 8);

    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 9], &sensorValue1, 4);
    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 13], &sensorValue2, 4);

    if (SocketClientSendPacket (socketSendBuff, HeaderOffsetsSecondEnd + 17) == false)
    {
        // error
        UART_PRINT("SocketClientSendSensorData sending packet_ERROR\n\r");
        return false;
 //       SocketClientReconnect ();
    }

    return true;
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
