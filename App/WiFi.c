//26.08.17
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"

#include "hw_types.h"
#include "rom_map.h"
#include "utils.h"

//#include "uart_if.h"
//#include "gpio_if.h"
//#include "network_if.h"
#include "wlan.h"
//#include "common.h"

#include "cc1310_interface.h"

#include "global.h"
#include "socketClient.h"
#include "socketProtocol.h"
#include "WiFi.h"
#include "OTA_Update.h" //TODO: DELETE
#include "prcm.h"

// HTTP Client lib
//#include <http/client/httpcli.h>
//#include <http/client/common.h>

#define APListSize  4
AP_Params_t APList [APListSize];


//TODO: DELETE
char acSendBuff[512];   // Buffer to send data
//struct sockaddr_in addr;
//HTTPCli_Struct g_cli;
//HTTPCli_Struct g_cli2;
#define HOST_NAME       "91.244.253.100"
#define HOST_PORT       (90)

#define PREFIX_BUFFER "/?sensor="
#define POST_BUFFER "&value="
#define POST_VALUE2 "&value2="
#define POST_VALUE3 "&value3="

//#define HOST_NAME2               "192.168.2.11"
//#define HOST_PORT2               (80)
//#define PREFIX_BUFFER2           "/snap.jpg"
#define HOST_NAME2               "91.244.253.100"
#define HOST_PORT2               (90)
#define PREFIX_BUFFER2           "/1.jpg"

#define SL_STOP_TIMEOUT         200

bool ConnectToAP ();

//----------------------------------------------------------------------------------------------------------------------------------
void SendError()
{
//    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
//    MAP_UtilsDelay(2000000);
//    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
}

//TODO: DELETE
//----------------------------------------------------------------------------------------------------------------------------------
bool SendNumber (char* text, int len, int value)
{
/*    long lRetVal;

     lRetVal = HTTPCli_connect(&g_cli, (struct sockaddr *)&addr, 0, NULL);
     if (lRetVal < 0)
     {
         SendError();
         SendNumber ("Error 2", strlen ("Error 2"), lRetVal);

         return false;
     }

     char* pcBufLocation;

      HTTPCli_Field fields[2] = {
                                  {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                  {NULL, NULL},
                                };
      //
      // Set request fields
      //
      HTTPCli_setRequestFields(&g_cli, fields);

      memset (acSendBuff, 0, strlen (acSendBuff));

      pcBufLocation = acSendBuff;
      strcpy(pcBufLocation, "/?text=");
      pcBufLocation += strlen("/?text=");

      memcpy (pcBufLocation, text, len);
      pcBufLocation += len;

      strcpy(pcBufLocation, ": ");
      pcBufLocation += strlen(": ");

      pcBufLocation += itoa (value, pcBufLocation);

      //
      // Make HTTP 1.1 GET request
      //
      lRetVal = HTTPCli_sendRequest(&g_cli, HTTPCli_METHOD_GET, acSendBuff, 0);


      if (lRetVal < 0)
      {
          SendError();
          SendNumber ("Error 3", strlen ("Error 3"), lRetVal);

          return false;
      }

      HTTPCli_disconnect (&g_cli);*/

      return true;
}




//TODO: DELETE
//----------------------------------------------------------------------------------------------------------------------------------
bool SendText (char* text, int len)
{
/*    if (g_appState != DEVICE_CONNECTED_AP)
        return false;

    long lRetVal;

    lRetVal = HTTPCli_connect(&g_cli, (struct sockaddr *)&addr, 0, NULL);
    if (lRetVal < 0)
    {
        SendError();
        SendNumber ("Error 4", strlen ("Error 4"), lRetVal);

        return false;
    }

    char* pcBufLocation;

     HTTPCli_Field fields[2] = {
                                 {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                 {NULL, NULL},
                               };
     //
     // Set request fields
     //
     HTTPCli_setRequestFields(&g_cli, fields);

     memset (acSendBuff, 0, strlen (acSendBuff));

     pcBufLocation = acSendBuff;
     strcpy(pcBufLocation, "/?text=");
     pcBufLocation += strlen("/?text=");

     memcpy (pcBufLocation, text, len);

     int s = 0;

     for (s = 0; s < len; s++)
     {
         if (pcBufLocation[s] == '/')
             pcBufLocation[s] = '.';
         if (pcBufLocation[s] == '?')
             pcBufLocation[s] = '.';
         if (pcBufLocation[s] == '&')
             pcBufLocation[s] = '.';
     }

     pcBufLocation += len;

     //
     // Make HTTP 1.1 GET request
     //
     lRetVal = HTTPCli_sendRequest(&g_cli, HTTPCli_METHOD_GET, acSendBuff, 0);


     if (lRetVal < 0)
     {
         SendError();
         SendNumber ("Error 5", strlen ("Error 5"), lRetVal);

         return false;
     }

     HTTPCli_disconnect (&g_cli);*/

     return true;
}

//*****************************************************************************
//
//! This function flush received HTTP response
//!
//! \param[in] cli - Instance of the HTTP connection
//!
//! \return o on success else -ve
//!
//*****************************************************************************
/*static int FlushHTTPResponse(HTTPCli_Handle cli)
{
    const char *ids[2] = {
                            HTTPCli_FIELD_NAME_CONNECTION, // App will get connection header value. all others will skip by lib
                            NULL
                         };
    char  buf[128];
    int id;
    int len = 1;
    bool moreFlag = 0;
    char ** prevRespFilelds = NULL;


    prevRespFilelds = HTTPCli_setResponseFields(cli, ids);

    // Read response headers
    while ((id = HTTPCli_getResponseField(cli, buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {
        if(id == 0)
        {
            if(!strncmp(buf, "close", sizeof("close")))
            {
                UART_PRINT("Connection terminated by server\n\r");
            }
        }
    }

    HTTPCli_setResponseFields(cli, (const char **)prevRespFilelds);

    while(1)
    {
        len = HTTPCli_readResponseBody(cli, buf, sizeof(buf) - 1, &moreFlag);
        ASSERT_ON_ERROR(len);

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n')
        {
            break;
        }

        if(!moreFlag)
        {
            break;
        }
    }
    return SUCCESS;
}*/

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
_i16 HttpClient_Connect(_u8 *ServerName, _u16 Port)
{
    _i32 Status;
    SlSockAddrIn_t  Addr;
    _i32 AddrSize;
    _i16 SockId = 0;
    _u32 ServerIp;
    SlTimeval_t tTimeout;
//    SlSocklinger_t linger;


    /* get host IP by name */
    Status = sl_NetAppDnsGetHostByName((_i8 *)ServerName, (_u16)strlen((const char *)ServerName), &ServerIp, SL_AF_INET);
    if(0 > Status )
    {
        UART_PRINT("HttpClient_Connect: ERROR sl_NetAppDnsGetHostByName, status=%d\r\n", Status);
        return -5;
    }


    UART_PRINT ("HttpClient_Connect: IP_ADDR=%u.%u.%u.%u\r\n", ((_u8 *)&ServerIp)[3]&0xff, ((_u8 *)&ServerIp)[2]&0xff, ((_u8 *)&ServerIp)[1]&0xff, ((_u8 *)&ServerIp)[0]&0xff);

    /* create socket */
    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(Port);
    Addr.sin_addr.s_addr = sl_Htonl(ServerIp);

    AddrSize = sizeof(SlSockAddrIn_t);

    SockId = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    if( SockId < 0 )
    {
        UART_PRINT("http_connect_server: ERROR Socket Open, status=%d\r\n", SockId);
        return -1;
    }

    /* set recieve timeout */
    tTimeout.tv_sec=2;
    tTimeout.tv_usec=0;
    Status = sl_SetSockOpt(SockId, SL_SOL_SOCKET, SL_SO_RCVTIMEO, &tTimeout, sizeof(SlTimeval_t));
    if( Status < 0 )
    {
        sl_Close(SockId);
        UART_PRINT("HttpClient_Connect: ERROR sl_SetSockOpt SL_SO_RCVTIMEO, status=%d\r\n", Status);
        return -2;
    }

    /* close the file socket with linger time of 1 seconds */
    /* It solves a case when the close socket failed after several seconds in the next OTA download retry */
/*    linger.l_onoff = 1;
    linger.l_linger = 1;
    Status = sl_SetSockOpt(SockId, SL_SOL_SOCKET, SL_SO_LINGER, &linger, sizeof(linger));
    if( Status < 0 )
    {
        sl_Close(SockId);
        UART_PRINT("HttpClient_Connect: ERROR sl_SetSockOpt SL_SO_LINGER, status=%d\r\n", Status);
        return -3;
    }*/

    /* connect to the server */
    Status = sl_Connect(SockId, ( SlSockAddr_t *)&Addr, (_u16)AddrSize);

    if ( Status < 0 )
    {
        sl_Close(SockId);
        UART_PRINT("HttpClient_Connect: ERROR Socket Connect, status=%ld\r\n", Status);
        return -4;
    }

    return SockId;
}

/* HTTP Get stcuture:
    GET <URI> HTTP/1.1
    Host: <Domain>
    Authorization: Bearer <Access Token>
*/
_i16 HttpClient_SendReq(_i16 SockId, _u8 *pHttpReqBuf, _u8 *pReqMethod, _u8 *pServerName, _u8 *pUriPrefix, _u8 *pUriVal, _u8 *pHdrName)
{
    _i16 Len;

    /* start with method GET/POST/PUT */
    strcpy((char *)pHttpReqBuf, (const char *)pReqMethod);

    /* fill uri_req_prefix */
    if (pUriPrefix && strlen((const char *)pUriPrefix))
    {
        strcat((char *)pHttpReqBuf, (const char *)pUriPrefix);
    }
    /* fill request URI */
    if (pUriVal && strlen((const char *)pUriVal))
    {
        strcat((char *)pHttpReqBuf, (const char *)pUriVal);
    }

    /* fill domain */
    strcat((char *)pHttpReqBuf, " HTTP/1.1\r\nHost: ");
    strcat((char *)pHttpReqBuf, (const char *)pServerName);
    strcat((char *)pHttpReqBuf, "\r\n");

    /* fill access_token */
    if (pHdrName && strlen((const char *)pHdrName))
    {
        strcat((char *)pHttpReqBuf, (const char *)pHdrName);
        strcat((char *)pHttpReqBuf, "\r\n");
    }

    strcat((char *)pHttpReqBuf, "\r\n\0");


    /* Send the prepared request */
    Len = sl_Send(SockId, pHttpReqBuf, (_i16)strlen((const char *)pHttpReqBuf), 0);

    return Len;
}

_u8 g_pHttpReqBuf [200];
_u8 socketRecvBuff [1500];
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

//--------------------------------------------------------------------------------------------------------------------------
bool GetJpg()
{
    _u8 serverName[] = HOST_NAME2;
    _i16 httpSockID = HttpClient_Connect(serverName, HOST_PORT2);

    if (httpSockID < 0)
    {
        UART_PRINT("GetJpg  (httpSockID < 0)  %d\r\n", httpSockID);
        return false;
    }

    _u8 pReqMethod[] = "GET ";
    _u8 pUriVal[] = PREFIX_BUFFER2;
    _u8 pHdrName[] = "Accept: text/html, application/xhtml+xml, */*\r\nAuthorization: admin:4rvRpbgX\r\n";
    _i16 len = HttpClient_SendReq (httpSockID, (_u8*) &g_pHttpReqBuf,  (_u8*)&pReqMethod,  (_u8*)&serverName, NULL,  (_u8*)&pUriVal,  (_u8*)&pHdrName);

    _i16 recvLen = sl_Recv(httpSockID, socketRecvBuff, sizeof (socketRecvBuff), 0);

    if (recvLen < 0)
    {
        UART_PRINT("GetJpg  (recvLen < 0)  %d\r\n", recvLen);
        sl_Close(httpSockID);
        return false;
    }

//    UART_PRINT("recvLen  %d\r\n", recvLen);
    UART_PRINT("GET HTTP: %d   %.*s\n\r", recvLen, recvLen, socketRecvBuff);


    char buff[255];
    sprintf (buff, "GET HTTP: %d   %.*s\n\r", recvLen, MIN (recvLen, sizeof (buff) - 30), socketRecvBuff);
    UART_PRINT (buff);
    SocketClientSendLog((char*) buff, LogPart_ClientRuntime, LogType_HTTPGetHeader);


    char *fileName = "Cottage. Cam 1";
    int packetLen = HeaderOffsetsSecondEnd + 8 + strlen ((char*) fileName); //+Type(1) + SubType(1) + FileType (1) + FileSubType (4) + StrLen (1)
    uint8_t *socketSendBuff = malloc (packetLen);

    socketSendBuff [HeaderOffsetsSecondEnd] = PType_File;
    socketSendBuff [HeaderOffsetsSecondEnd + 1] = PSubType_FileStart;
    socketSendBuff [HeaderOffsetsSecondEnd + 2] = 1;
    memset (&socketSendBuff [HeaderOffsetsSecondEnd + 3], 0, 4);

    socketSendBuff [HeaderOffsetsSecondEnd + 7] = strlen ((char*)fileName);

    memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 8], fileName, strlen ((char*) fileName));

    if (SocketClientSendPacket (socketSendBuff, packetLen) == false)
    {
        free (socketSendBuff);
        sl_Close(httpSockID);
        return false;
    }

    free (socketSendBuff);

    int bytesReceived = 0;
    int partNum = 0;
    while(1)
    {
        len = sl_Recv(httpSockID, socketRecvBuff, sizeof (socketRecvBuff), 0);
        if(len <= 0)
        {
            break;
        }

        partNum++;

        UART_PRINT("partNum: %d    len: %d\n\r", partNum, len);

        packetLen = HeaderOffsetsSecondEnd + 2 + len; //+Type(1) + SubType(1) + data
        socketSendBuff = malloc (packetLen);

        socketSendBuff [HeaderOffsetsSecondEnd] = PType_File;
        socketSendBuff [HeaderOffsetsSecondEnd + 1] = PSubType_FileContinue;

        memcpy (&socketSendBuff [HeaderOffsetsSecondEnd + 2], socketRecvBuff, len);

        if (SocketClientSendPacket (socketSendBuff, packetLen) == false)
        {
            free (socketSendBuff);
            sl_Close(httpSockID);
            return false;
        }

        free (socketSendBuff);

        bytesReceived +=len;
    }

    packetLen = HeaderOffsetsSecondEnd + 2; //+Type(1) + SubType(1)
    socketSendBuff = malloc (packetLen);

    socketSendBuff [HeaderOffsetsSecondEnd] = PType_File;
    socketSendBuff [HeaderOffsetsSecondEnd + 1] = PSubType_FileEnd;

    if (SocketClientSendPacket (socketSendBuff, packetLen) == false)
    {
        free (socketSendBuff);
        sl_Close(httpSockID);
        return false;
    }

    free (socketSendBuff);

    {
        char buff[100];
        sprintf (buff, "GetJpg.Total bytes received: %d", bytesReceived);
        SocketClientSendLog(buff, LogPart_ClientRuntime, LogType_None);
    }

    UART_PRINT("Total bytes received: %d\n\r", bytesReceived);

    sl_Close(httpSockID);

    return true;

}

/*!
\brief          This function handles general events
\param[in]      pDevEvent - Pointer to stucture containing general event info
\return         None
*/
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(NULL == pDevEvent) return;
    switch(pDevEvent->Id)
    {
        default:
        {
            if (pDevEvent->Data.Error.Code == SL_ERROR_LOADING_CERTIFICATE_STORE)
            {
                /* Ignore it */
                UART_PRINT("GeneralEventHandler: EventId=%d, SL_ERROR_LOADING_CERTIFICATE_STORE, ErrorCode=%d\r\n", pDevEvent->Id, pDevEvent->Data.Error.Code);
                break;
            }
            UART_PRINT("Received unexpected General Event with code [0x%x]\n\r", pDevEvent->Data.Error.Code);
//            SignalEvent(APP_EVENT_ERROR);
        }
        break;
    }
}

/*
 *  \brief      This function handles WLAN async events
 *  \param[in]  pWlanEvent - Pointer to the structure containg WLAN event info
 *  \return     None
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    SlWlanEventData_u *pWlanEventData = NULL;

    if (NULL == pWlanEvent) return;

    pWlanEventData = &pWlanEvent->Data;

    switch(pWlanEvent->Id)
    {
        case SL_WLAN_EVENT_CONNECT:
        {
            UART_PRINT("STA connected to AP %s, ", pWlanEvent->Data.Connect.SsidName);

            UART_PRINT("BSSID is %02x:%02x:%02x:%02x:%02x:%02x\n\r",
                    pWlanEvent->Data.Connect.Bssid[0],
                    pWlanEvent->Data.Connect.Bssid[1],
                    pWlanEvent->Data.Connect.Bssid[2],
                    pWlanEvent->Data.Connect.Bssid[3],
                    pWlanEvent->Data.Connect.Bssid[4],
                    pWlanEvent->Data.Connect.Bssid[5]);

//            SignalEvent(APP_EVENT_CONNECTED);
        }
        break;

        case SL_WLAN_EVENT_DISCONNECT:
        {
            SlWlanEventDisconnect_t *pDiscntEvtData = NULL;
            pDiscntEvtData = &pWlanEventData->Disconnect;

            /** If the user has initiated 'Disconnect' request, 'ReasonCode'
              * is SL_USER_INITIATED_DISCONNECTION
              */
            if(SL_WLAN_DISCONNECT_USER_INITIATED == pDiscntEvtData->ReasonCode)
            {
                UART_PRINT("Device disconnected from the AP on request\n\r");
            }
            else
            {
                UART_PRINT("Device disconnected from the AP on an ERROR\n\r");
            }
            //if (ConnectToAP() == false)
                RebootMCU();

//            SignalEvent(APP_EVENT_DISCONNECT);
        }
        break;

        case SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED:
            UART_PRINT(" [Provisioning] Profile Added: SSID: %s\r\n", pWlanEvent->Data.ProvisioningProfileAdded.Ssid);
            if(pWlanEvent->Data.ProvisioningProfileAdded.ReservedLen > 0)
            {
                UART_PRINT(" [Provisioning] Profile Added: PrivateToken:%s\r\n", pWlanEvent->Data.ProvisioningProfileAdded.Reserved);
            }
            break;

        case SL_WLAN_EVENT_PROVISIONING_STATUS:
        {
                switch(pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus)
                {
                case SL_WLAN_PROVISIONING_GENERAL_ERROR:
                case SL_WLAN_PROVISIONING_ERROR_ABORT:
                case SL_WLAN_PROVISIONING_ERROR_ABORT_INVALID_PARAM:
                case SL_WLAN_PROVISIONING_ERROR_ABORT_HTTP_SERVER_DISABLED:
                case SL_WLAN_PROVISIONING_ERROR_ABORT_PROFILE_LIST_FULL:
                case SL_WLAN_PROVISIONING_ERROR_ABORT_PROVISIONING_ALREADY_STARTED:
                    UART_PRINT(" [Provisioning] Provisioning Error status=%d\r\n",pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
//                      SignalEvent(APP_EVENT_ERROR);
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_NETWORK_NOT_FOUND:
                    UART_PRINT(" [Provisioning] Profile confirmation failed: network not found\r\n");
//                      SignalEvent(APP_EVENT_PROVISIONING_STARTED);
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_CONNECTION_FAILED:
                    UART_PRINT(" [Provisioning] Profile confirmation failed: Connection failed\r\n");
//                      SignalEvent(APP_EVENT_PROVISIONING_STARTED);
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_CONNECTION_SUCCESS_IP_NOT_ACQUIRED:
                    UART_PRINT(" [Provisioning] Profile confirmation failed: IP address not acquired\r\n");
//                      SignalEvent(APP_EVENT_PROVISIONING_STARTED);
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS_FEEDBACK_FAILED:
                    UART_PRINT(" [Provisioning] Profile Confirmation failed (Connection Success, feedback to Smartphone app failed)\r\n");
//                      SignalEvent(APP_EVENT_PROVISIONING_STARTED);
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS:
                    UART_PRINT(" [Provisioning] Profile Confirmation Success!\r\n");
 //                   SignalEvent(APP_EVENT_PROVISIONING_SUCCESS);
                    break;

                case SL_WLAN_PROVISIONING_AUTO_STARTED:
                    UART_PRINT(" [Provisioning] Auto-Provisioning Started\r\n");
//                      SignalEvent(APP_EVENT_PROVISIONING_STARTED);
                      break;

                case SL_WLAN_PROVISIONING_STOPPED:
                    UART_PRINT("\r\n Provisioning stopped:");
//                    UART_PRINT(" Current Role: %d\r\n",[pWlanEvent->Data.ProvisioningStatus.Role);
                    if(ROLE_STA == pWlanEvent->Data.ProvisioningStatus.Role)
                    {
//                        UART_PRINT("                       WLAN Status: %d\r\n",pWlanEvent->Data.ProvisioningStatus.WlanStatus);

                        if(SL_WLAN_STATUS_CONNECTED == pWlanEvent->Data.ProvisioningStatus.WlanStatus)
                        {

                            UART_PRINT("                       Connected to SSID: %s\r\n",pWlanEvent->Data.ProvisioningStatus.Ssid);
//                              SignalEvent(APP_EVENT_PROVISIONING_STOPPED);
                        }
                        else
                        {
//                              SignalEvent(APP_EVENT_PROVISIONING_STARTED);
                        }
                    }

//                    g_StopInProgress = 0;
                    break;

                case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNCED:
                    UART_PRINT(" [Provisioning] Smart Config Synced!\r\n");
                    break;

                case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNC_TIMEOUT:
                    UART_PRINT(" [Provisioning] Smart Config Sync Timeout!\r\n");
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_WLAN_CONNECT:
                    UART_PRINT(" [Provisioning] Profile confirmation: WLAN Connected!\r\n");
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_IP_ACQUIRED:
                    UART_PRINT(" [Provisioning] Profile confirmation: IP Acquired!\r\n");
                    break;

                case SL_WLAN_PROVISIONING_EXTERNAL_CONFIGURATION_READY:
                    UART_PRINT(" [Provisioning] External configuration is ready! \r\n");
                    break;

                default:
                    UART_PRINT(" [Provisioning] Unknown Provisioning Status: %d\r\n",pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
                    break;
                }
        }
    }


}

/*!
 *  \brief      This function handles resource request
 *  \param[in]  pNetAppRequest - Contains the resource requests
 *  \param[in]  pNetAppResponse - Should be filled by the user with the
 *                                relevant response information
 *  \return     None
 */
void SimpleLinkNetAppRequestHandler(SlNetAppRequest_t  *pNetAppRequest,
                                    SlNetAppResponse_t *pNetAppResponse)
{
    /* Unused in this application */
    UART_PRINT("Unexpected NetApp request event \n\r");
//    SignalEvent(APP_EVENT_ERROR);
}

/*!
 *  \brief       The Function Handles the Fatal errors
 *  \param[in]  pFatalErrorEvent - Contains the fatal error data
 *  \return     None
 */
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{

            UART_PRINT("[ERROR] - FATAL ERROR: Abort NWP event detected: AbortType=%d, AbortData=0x%x\n\r",slFatalErrorEvent->Data.DeviceAssert.Code,slFatalErrorEvent->Data.DeviceAssert.Value);

}


/*!
 *  \brief      This function gets triggered when HTTP Server receives
 *              application defined GET and POST HTTP tokens.
 *  \param[in]  pHttpServerEvent Pointer indicating http server event
 *  \param[in]  pHttpServerResponse Pointer indicating http server response
 *  \return     None
 */
void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pHttpEvent,
                                  SlNetAppHttpServerResponse_t *pHttpResponse)
{
    /* Unused in this application */
    UART_PRINT("Unexpected HTTP server event \n\r");

}





/*!
 *  \brief      This function handles socket events indication
 *  \param[in]  pSock - Pointer to the stucture containing socket event info
 *  \return     None
 */
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{

}

void SimpleLinkNetAppRequestMemFreeEventHandler (uint8_t *buffer)
{
  // do nothing...
}

void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse)
{
  // do nothing...
}


//--------------------------------------------------------------------------------------------------------------------------
/*!
 *  \brief      This function handles network events such as IP acquisition, IP
 *              leased, IP released etc.
 * \param[in]   pNetAppEvent - Pointer to the structure containing acquired IP
 * \return      None
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    SlNetAppEventData_u *pNetAppEventData = NULL;

    if(NULL == pNetAppEvent)
        return;

    pNetAppEventData = &pNetAppEvent->Data;

    switch(pNetAppEvent->Id)
    {
        case SL_NETAPP_EVENT_IPV4_ACQUIRED:
        {
            UART_PRINT("IPv4 acquired: IP = %d.%d.%d.%d\n\r",\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,3),\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,2),\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,1),\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,0));
            UART_PRINT("Gateway = %d.%d.%d.%d\n\r",\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,3),\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,2),\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,1),\
                (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,0));
        }
        break;

        case SL_NETAPP_EVENT_IPV4_LOST:
        case SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT:
        {
            UART_PRINT("IPv4 lost Id or timeout, Id [0x%x]!!!\n\r", pNetAppEvent->Id);
        }
        break;

        default:
        {
            UART_PRINT("Unexpected NetApp event with Id [0x%x] \n\r", pNetAppEvent->Id);
        }
        break;
    }

    g_lastSlEvent = *pNetAppEvent;
}

//--------------------------------------------------------------------------------------------------------------------------
bool ConnectToAP ()
{
//    Network_IF_ResetMCUStateMachine();
    UART_PRINT ("ConnectToAP ()\n\r");

    long lRetVal = -1;
//    while (lRetVal < 0)
    {
        lRetVal = sl_Start(0, 0, 0);
//        Platform_Sleep(100);

        if (lRetVal != ROLE_STA)
        {
            UART_PRINT("SimpleLinkInitCallback: started in role %d, set the requested role %d\n\r", lRetVal, ROLE_STA);
            lRetVal = sl_WlanSetMode(ROLE_STA);
            LOG_ON_ERROR(lRetVal);
            lRetVal = sl_Stop(SL_STOP_TIMEOUT);
            LOG_ON_ERROR(lRetVal);
            lRetVal = sl_Start(0, 0, 0);
            LOG_ON_ERROR(lRetVal);
            if (ROLE_STA != lRetVal)
            {
                UART_PRINT("SimpleLinkInitCallback: error setting role %d, status=%d\n\r", ROLE_STA, lRetVal);
                return false;
            }
            UART_PRINT("SimpleLinkInitCallback: restarted in role %d\n\r", lRetVal);
        }
    }

    UART_PRINT("SimpleLinkInitCallback: started in role %d\n\r", lRetVal);

//    LedTimerConfigNStart();

    _u8 policy = 1;

    _u16 scanInterval = 600;

    sl_WlanPolicySet (SL_WLAN_POLICY_SCAN, policy, (_u8*) scanInterval, sizeof(scanInterval));

    // delay 1 second to verify scan is started
    Platform_Sleep(1000);

    SlWlanNetworkEntry_t netEntries[20];
    _i16 resultsCount = sl_WlanGetNetworkList (0, 20, &netEntries[0]);

    if (resultsCount == 0)
    {
        UART_PRINT ("ConnectToAP ()  (resultsCount == 0)\n\r");
        return false;
    }

    int s;
    int s1;
    SlWlanNetworkEntry_t temp;

    for(s = 0; s < resultsCount; s++)
        for(s1 = s + 1; s1 < resultsCount; s1++)
        {
            if (netEntries[s].Rssi < netEntries[s1].Rssi)
            {
                temp = netEntries[s];
                netEntries[s] = netEntries[s1];
                netEntries[s1] = temp;
            }

        }

    for(s = 0; s < resultsCount; s++)
        UART_PRINT ("%s   %d\n\r", netEntries[s].Ssid, netEntries[s].Rssi);

    bool connectSuccess = false;

    for(s = 0; s < resultsCount; s++)
    {
        for (s1 = 0; s1 < APListSize; s1++)
        {
            if (strcmp ((char*) netEntries[s].Ssid, APList[s1].ssid) == 0)
            {
                int loop = 0;

                lRetVal = sl_WlanConnect((_i8*) APList[s1].ssid, strlen(APList[s1].ssid), 0, &APList[s1].secParams, 0);
                if (lRetVal < 0)
                    continue;

                for (loop = 0; loop < 100; loop++)
                {
                    UART_PRINT ("%d:  %d   %s   \n\r", loop,g_lastSlEvent.Id, APList[s1].ssid);


//                    if (IS_IP_ACQUIRED(Network_IF_CurrentMCUState()))
                    if (g_lastSlEvent.Id == SL_NETAPP_EVENT_IPV4_ACQUIRED)
                    {
                        _u16 macAddressLen = 6;
                        _u16 ConfigOpt = 0;
                        sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET,&ConfigOpt,&macAddressLen,(_u8 *)macAddressVal);

                        if (SocketClientConnect() == true)
                            connectSuccess = true;
                        else
                        {
                            sl_WlanDisconnect();

                            int loop2;
                            for (loop2 = 0; loop2 < 300; loop2++)
                            {
//                                if (IS_CONNECTED(Network_IF_CurrentMCUState()) == false)
//                                    break;
                                Platform_Sleep(10);
                            }
                            connectSuccess = false;
                        }

                        break;
                    }
                    Platform_Sleep(100);
                }

                if (connectSuccess == true)
                    break;
            }
        }
        if (connectSuccess == true)
            break;
    }



    //
    // Disable the LED blinking Timer as Device is connected to AP
    //
//    LedTimerDeinitStop();

//    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);

    return connectSuccess;
}

//--------------------------------------------------------------------------------------------------------------------------
void FillAPList ()
{
    APList[0].ssid = "Avh-2 (asus)";
    APList[0].secParams.Key = "89172776060";
    APList[0].secParams.KeyLen = strlen ((char*) APList[0].secParams.Key);
    APList[0].secParams.Type = SL_WLAN_SEC_TYPE_WPA_WPA2;

    APList[1].ssid = "Avh";
    APList[1].secParams.Key = "89172776060";
    APList[1].secParams.KeyLen = strlen ((char*) APList[1].secParams.Key);
    APList[1].secParams.Type = SL_WLAN_SEC_TYPE_WPA_WPA2;

    APList[2].ssid = "iPhone Psilon";
    APList[2].secParams.Key = "89172776060";
    APList[2].secParams.KeyLen = strlen ((char*) APList[2].secParams.Key);
    APList[2].secParams.Type = SL_WLAN_SEC_TYPE_WPA_WPA2;

    APList[3].ssid = "VarS";
    APList[3].secParams.Key = "89172979137";
    APList[3].secParams.KeyLen = strlen ((char*) APList[3].secParams.Key);
    APList[3].secParams.Type = SL_WLAN_SEC_TYPE_WPA_WPA2;
}


//*****************************************************************************
long Network_IF_GetHostIP( char* pcHostName,unsigned long * pDestinationIP )
{
    long lStatus = 0;

    lStatus = sl_NetAppDnsGetHostByName((signed char *) pcHostName,
                                            strlen(pcHostName),
                                            pDestinationIP, SL_AF_INET);
//    ASSERT_ON_ERROR(lStatus);

    UART_PRINT("Get Host IP succeeded.\n\rHost: %s IP: %d.%d.%d.%d \n\r\n\r",
                    pcHostName, SL_IPV4_BYTE(*pDestinationIP,3),
                    SL_IPV4_BYTE(*pDestinationIP,2),
                    SL_IPV4_BYTE(*pDestinationIP,1),
                    SL_IPV4_BYTE(*pDestinationIP,0));
    return lStatus;

}

//****************************************************************************
//
//! Reboot the MCU by requesting hibernate for a short duration
//!
//! \return None
//
//****************************************************************************
void RebootMCU()
{
    UART_PRINT("RebootMCU() \n\r");
//    return;
  //
  // Configure hibernate RTC wakeup
  //
  PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);

  //
  // Delay loop
  //
  MAP_UtilsDelay(8000000);

  //
  // Set wake up time
  //
  PRCMHibernateIntervalSet(330);

  //
  // Request hibernate
  //
  PRCMHibernateEnter();

  //
  // Control should never reach here
  //
  while(1)
  {

  }
}


//--------------------------------------------------------------------------------------------------------------------------
void* WiFi_Task(void *pvParameters)
{
    long lRetVal = -1;

    unsigned long ulDestinationIP;

    //
    // Configure LED
    //
//    GPIO_IF_LedConfigure(LED1|LED2|LED3);

//    GPIO_IF_LedOff(MCU_ALL_LED_IND);
//    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);

    FillAPList ();
    ConnectToAP ();

//    StartBlinkTimer();

    //
    // Get the serverhost IP address using the DNS lookup
    //
    lRetVal = Network_IF_GetHostIP((char*)HOST_NAME, &ulDestinationIP);
    if(lRetVal < 0)
    {
        UART_PRINT("DNS lookup failed. %d \n\r",lRetVal);
    }

    g_appState = DEVICE_CONNECTED_AP;

    char text[] = "WIFI Cam started      ";

//    itoa (APPLICATION_VERSION, &text[18]);

    SendText (text, sizeof (text));

    sendInProgress = true;
    GetJpg();
    sendInProgress = false;

    Platform_Sleep(200);

    int aliveSendPeriodMS = 5 * 100;
    int lastAliveSendMS = 0;
    int jpgSendPeriodMS = 40 * 100;
    int lastJpgSendMS = 0;
//    int wifiAliveSensorIndex = 32;

    while (1)
    {
        if (messageCount == 0)
        {
            Platform_Sleep(10);
            lastAliveSendMS++;
            lastJpgSendMS++;

            if (lastAliveSendMS > aliveSendPeriodMS)
            {
                lastAliveSendMS = 0;

 //               if (!(IS_CONNECTED (Network_IF_CurrentMCUState())))
   //                 RebootMCU();

                SocketClientPing();


                char sensorMac [8];
                sensorMac[0] = 0x00;
                sensorMac[1] = 0x12;
                sensorMac[2] = 0x4B;
                sensorMac[3] = 0x00;
                sensorMac[4] = 0x11;
                sensorMac[5] = 0x6D;
                sensorMac[6] = 0xAE;
                sensorMac[7] = 0x44;

                SocketClientSendSensorData (sensorMac, (float) ping, 0);

/*                lRetVal = HTTPCli_connect(&g_cli, (struct sockaddr *)&addr, 0, NULL);
                if (lRetVal < 0)
                    RebootMCU();

                SendSensorData (wifiAliveSensorIndex, 1, 0, 0);

                HTTPCli_disconnect (&g_cli);*/
            }

            if (lastJpgSendMS > jpgSendPeriodMS)
            {
                lastJpgSendMS = 0;

                sendInProgress = true;
                if (GetJpg() == false)
                    RebootMCU();


                sendInProgress = false;

//                osi_Sleep(10 * 1000);

//                SocketClientReconnect();

//                osi_Sleep(5 * 1000);
//                RebootMCU();
            }

        }
        else
        {

/*            if (cloudSensors[i].cloudValueIndex1 != 0)
            {
 //               SendNumber ("messageBuff.UARTIndex", strlen ("messageBuff.UARTIndex"), messageBuff [messageIndex].UARTIndex);

                SendSensorData (cloudSensors[i].cloudValueIndex1, (signed char) messageBuff [messageIndex].message[0],
                                messageBuff [messageIndex].radioIndex, messageBuff [messageIndex].UARTIndex);
                osi_Sleep(200);
            }*/

            if (messageIndex == MAX_MESSAGE_COUNT - 1)
                messageIndex = 0;
            else
                messageIndex++;

            messageCount--;
        }
    }
//    HTTPCli_destruct(&cli);
}


