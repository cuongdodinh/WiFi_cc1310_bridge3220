//26.08.17
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"

#include "hw_types.h"
#include "rom_map.h"
#include "utils.h"

#include "wlan.h"

#include "cc1310_interface.h"

#include "global.h"
#include "socketClient.h"
#include "socketProtocol.h"
#include "WiFi.h"
#include "OTA_Update.h"
#include "prcm.h"

#define APListSize  4
AP_Params_t APList [APListSize];

#define HOST_NAME       "91.244.253.100"
#define HOST_PORT       (90)

//#define HOST_NAME2               "192.168.2.11"
//#define HOST_PORT2               (80)
//#define PREFIX_BUFFER2           "/snap.jpg"
#define HOST_NAME2               "91.244.253.100"
#define HOST_PORT2               (90)
#define PREFIX_BUFFER2           "/1.jpg"

#define SL_STOP_TIMEOUT         200

bool ConnectToAP ();


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

//        UART_PRINT("partNum: %d    len: %d\n\r", partNum, len);

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

    sl_Stop(SL_STOP_TIMEOUT );

    PRCMHibernateCycleTrigger();
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
    if (ConnectToAP () == false)
        RebootMCU();
    else
    {
        if (Ota_GetPendingCommitState() == true)
        {
            UART_PRINT("[OTA] committing new ota download... \n\r");
            if (Ota_Commit() < 0)
            {
                UART_PRINT("[Local ota task] failed to commit new download, reverting to previous copy by reseting the device \n\r");
                RebootMCU();

                return 0;
            }
            UART_PRINT("[OTA] commit succeeded \n\r");

            /* need to stop the WDT so MCU is not reset */
            PRCMPeripheralReset(PRCM_WDT);
        }
    }


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

    Platform_Sleep(200);

    int aliveSendPeriodMS = 5 * 100;
    int lastAliveSendMS = 0;
    int jpgSendPeriodMS = 40 * 100;
    int lastJpgSendMS = 0;
//    int wifiAliveSensorIndex = 32;

    while (1)
    {
        if (SocketClientProcessRecv () < 0)
            RebootMCU();

        if (messageCount == 0)
        {
            Platform_Sleep(10);
            lastAliveSendMS++;
            lastJpgSendMS++;

            if (lastAliveSendMS > aliveSendPeriodMS)
            {
                lastAliveSendMS = 0;

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

                SocketClientSendSensorData (sensorMac, (float) g_ping, 0);
            }

            if (lastJpgSendMS > jpgSendPeriodMS)
            {
                lastJpgSendMS = 0;

//                if (GetJpg() == false)
//                    RebootMCU();

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
}


