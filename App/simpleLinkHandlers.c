#include "global.h"
#include "WiFi.h"
#include "uart_term.h"

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
//                RebootMCU();

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
