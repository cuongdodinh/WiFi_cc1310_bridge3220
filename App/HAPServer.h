//18.10.17

#ifndef APP_HAPSERVER_H_
#define APP_HAPSERVER_H_

#include <ti/hap/ip/HAPEngine.h>

#define NUMIDBYTES 6
#define NAME_LEN 32
#define HAPDEVICEIDLEN 17 + 1 /* +1 for null term char */
#define APP_DATA "blink_app_data"
#define ACC_NAME "bridge"
#define Board_MFI_ADDR               (0x11)
#define HAPSTOPTIMEOUT 1000 /* milliseconds */

typedef struct App_Data {
    unsigned char deviceID[NUMIDBYTES];
    char accName[NAME_LEN + 1];
} App_Data;

extern HAPEngine_Handle HAPEngineHandle;

void* HAPServer_Task(void *arg);



#endif /* APP_HAPSERVER_H_ */
