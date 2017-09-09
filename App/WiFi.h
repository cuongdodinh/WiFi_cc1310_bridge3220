//26.08.17

#ifndef APP_WIFI_H_
#define APP_WIFI_H_

void* WiFi_Task(void *pvParameters);
bool SendText (char* text, int len);

typedef struct AP_Params {
    char *ssid;
    SlWlanSecParams_t secParams;
} AP_Params_t;

#endif /* APP_WIFI_H_ */
