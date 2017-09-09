//26.08.17

#ifndef OTA_UPDATE_H_
#define OTA_UPDATE_H_

#include "simplelink.h"

#define APPLICATION_VERSION     8

#define OTA_PREFIX_BUFFER           "/WiFiCam.bin"
#define OTA_VERINFO_PREFIX_BUFFER           "/WiFiCam.txt"
#define OTA_HOST_NAME               "91.244.253.100"
#define OTA_HOST_PORT               (90)

#define FILE_NAME1               "/sys/mcuimg2.bin"
#define FILE_NAME2               "/sys/mcuimg3.bin"

void OTA_Update_Task (void *pvParameters);
void RebootMCU();

#endif /* OTA_UPDATE_H_ */
