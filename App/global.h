//26.08.17

#ifndef APP_GLOBAL_H_
#define APP_GLOBAL_H_

#define APPLICATION_VERSION     15

#include <stdbool.h>
#include <semaphore.h>
#include "platform.h"
#include "uart_term.h"
#include "netapp.h"

extern SlNetAppEvent_t g_lastSlEvent;

// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes

    DEVICE_STARTED = -0x7D0,
    DEVICE_CONNECTED_AP = DEVICE_STARTED - 1,
    DEVICE_CONNECTED_SERVER = DEVICE_CONNECTED_AP - 1,
    DEVICE_CONNECTED_SERVER_AUTH = DEVICE_CONNECTED_SERVER - 1,
    SERVER_CONNECTION_ERROR = DEVICE_CONNECTED_SERVER_AUTH - 1,
    DEVICE_START_FAILED = SERVER_CONNECTION_ERROR - 1,
    INVALID_HEX_STRING = DEVICE_START_FAILED - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_OPEN_FAILED = FORMAT_NOT_SUPPORTED - 1,
    FILE_WRITE_ERROR = FILE_OPEN_FAILED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,
    SERVER_CONNECTION_FAILED = INVALID_FILE - 1,
    GET_HOST_IP_FAILED = SERVER_CONNECTION_FAILED  - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

extern e_AppStatusCodes g_appState;

#define ERR_LOG(x) LogError("Error [%d] at line [%d] in function [%s]  \n\r",x,__LINE__,__FUNCTION__)
#define LOG_ERROR2(x,y) LogError("Error [%d, %d] at line [%d] in function [%s]  \n\r",x,y,__LINE__,__FUNCTION__)

#define LOG_ON_ERROR(error_code)\
            {\
                 if(error_code < 0) \
                   {\
                        ERR_LOG(error_code);\
                 }\
            }

#define LOG_NON_ZERO(error_code)\
            {\
                 if(error_code != 0) \
                   {\
                        ERR_LOG(error_code);\
                 }\
            }

#define LOG_ASSERT(param)\
            {\
                 if(!(param)) \
                   {\
                        ERR_LOG(param);\
                 }\
            }

#define LOG_ERROR(error_code)\
            {\
                        ERR_LOG(error_code);\
            }
//TODO: DELETE******************************************/////////~~~~~~~~~~~~~~~
#define MAX_BUFF_SIZE           1460
#define SUCCESS                 0

#define _u8 unsigned char
#define _u16 unsigned short
#define _u64 unsigned long

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

extern bool sendInProgress;
extern unsigned char g_buff[MAX_BUFF_SIZE+1];
extern _u8 macAddressVal[6];
extern sem_t HAPInitSem, HAPStopSem, WiFiConnectSem, btnSem;
extern int timeSinceStartup;

long Network_IF_GetHostIP( char* pcHostName,unsigned long * pDestinationIP );

void BlinkLED(char period);
void RebootMCU();
void LogError (const char *pcFormat, ...);

#endif /* APP_GLOBAL_H_ */
