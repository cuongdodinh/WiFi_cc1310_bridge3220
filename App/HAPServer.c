//18.10.17

#include <stdio.h>
#include "global.h"
#include "socketClient.h"

#include <ti/hap/HAPAcc.h>
#include <ti/drivers/storage/nvs.h>
#include <ti/mfi/mfiauth.h>
#include <ti/rng/rng.h>
#include "led.h"

#include "HAPServer.h"

static MFiAuth_Handle mfih = { .handle = NULL };
static HAPEngine_SrvStatus bridge_identifyFxn(unsigned int aId);

char deviceIDBuf[HAPDEVICEIDLEN];
App_Data appData;
HAPEngine_Handle HAPEngineHandle;

HAPEngine_AccessoryDesc accessory_services = {
    NULL, NULL, NULL, NULL, NULL, NULL, /* initialized at runtime */
    &bridge_identifyFxn,
    1,
    0,
    NULL
};

HAPEngine_ServiceDesc *accessory_serviceArray_led[] = {
    (HAPEngine_ServiceDesc *)&ledServiceDesc
};

HAPEngine_AccessoryDesc accessory_services_led = {
    NULL, NULL, NULL, NULL, NULL, NULL, /* initialized at runtime */
    &bridge_identifyFxn,
    4,
    sizeof(accessory_serviceArray_led)/sizeof(accessory_serviceArray_led[0]),
    &accessory_serviceArray_led
};

//------------------------------------------------------------------------------------------------------------------------
int accessory_init(HAPEngine_AccessoryDesc *a, char *name)
{
    a->name = name;
    a->manufacturer = "iHome";
    a->model = "BBCDE";
    a->serialNumber = "72345";
    a->fwRevision = "1";
    a->hwRevision = "1";

    return (0);
}

//------------------------------------------------------------------------------------------------------------------------
int accessory_init_led(HAPEngine_AccessoryDesc *a, char *name)
{
    a->name = name;
    a->manufacturer = "iHome";
    a->model = "FBCDE";
    a->serialNumber = "62345";
    a->fwRevision = "1";
    a->hwRevision = "1";

    return (0);
}

//------------------------------------------------------------------------------------------------------------------------
static HAPEngine_SrvStatus bridge_identifyFxn(unsigned int aId)
{
 //   int i;

/*    for (i = 0; i < 3; i++) {
        LED_identifyRoutine();
        usleep(500000);
    }
    LED_restoreState();*/

    return (HAPEngine_SrvStatus_EOK);
}

//------------------------------------------------------------------------------------------------------------------------
void appInit(App_Data *data, bool reset)
{
    unsigned int size = sizeof(App_Data);
    int status;

    if (reset || NVS_get(APP_DATA, data, 0, &size) != NVS_EOK) {
        RNG_setRandomSeed(NULL);
        RNG_getRandomBytes(data->deviceID, NUMIDBYTES);
        strcpy(data->accName, ACC_NAME);
        status = NVS_set(APP_DATA, data, 0, sizeof(App_Data), 0);
        LOG_ASSERT (status == NVS_EOK);
    }
}

//------------------------------------------------------------------------------------------------------------------------
int GetSetupCodeFxn(char *setupCode, uint8_t *salt, uint8_t *verifier)
{
    setupCode[0]  = '1';
    setupCode[1]  = '1';
    setupCode[2]  = '1';
    setupCode[3]  = '-';
    setupCode[4]  = '2';
    setupCode[5]  = '2';
    setupCode[6]  = '-';
    setupCode[7]  = '3';
    setupCode[8]  = '3';
    setupCode[9] = '3';
    setupCode[10] = '\0';

    return (HAPEngine_RAWSETUPCODE);
}

//------------------------------------------------------------------------------------------------------------------------
void *HAPServer_Task(void *arg)
{
    int status;
    int mfiStatus = -2;
    HAPEngine_AccessoryDesc *accs[4];
    HAPEngine_Params params;


    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
    UART_PRINT("HAPServer_Task start \n\r");



    /* open controller for the MFi chip */
    mfih.handle = I2C_open(CC3220SF_LAUNCHXL_I2C0, NULL);
    LOG_ASSERT (mfih.handle != NULL);
//    mfiStatus = MFiAuth_setDevice(&mfih, Board_MFI_ADDR);

    UART_PRINT ("MFiAuth: %d\n\r", mfiStatus);

    sl_Start(NULL,NULL,NULL);

    /* Init HAPEngine */
    HAPEngine_init();

    /* Has to be called after HAPEngine_init since NVS needs to initialized */
    appInit(&appData, false);

    sl_Stop(200);

    sem_post (&HAPInitSem);

    sem_wait (&WiFiConnectSem);

    UART_PRINT("Accessory Name: %s\n\r", appData.accName);

    GPIO_enableInt(Board_BUTTON0);

    /* save run-time supplied info in persistent storage */
//    status = NVS_set(APP_DATA, &appData, 0, sizeof(App_Data), 0);
//    assert(status == NVS_EOK);

    /* Initialize accessory descriptor with run-time supplied info */
    status = accessory_init(&accessory_services, appData.accName);
    LOG_ASSERT (status == HAPEngine_EOK);
    HAPEngine_Params_init(&params);

    /* Conditionally add MFi-based pairing (if the chip is detected) */
    if (mfiStatus == 0)
        params.mfiRespAndCertFxn = &MFiAuth_getRespAndCert;

/*    status = accessory_init_leak(&accessory_services_leak, "leakSensor");
    LOG_ASSERT(status == HAPEngine_EOK);

    status = accessory_init_temp(&accessory_services_temp, "tempSensor");
    LOG_ASSERT(status == HAPEngine_EOK);*/

    status = accessory_init_led(&accessory_services_led, "led");
    LOG_ASSERT(status == HAPEngine_EOK);

//    GPIO_write(Board_GPIO_LED1, Board_GPIO_LED_ON);

    accs[0] = &accessory_services;
//    accs[1] = &accessory_services_leak;
//    accs[2] = &accessory_services_temp;
    accs[1] = &accessory_services_led;

    do {

        snprintf(deviceIDBuf, HAPDEVICEIDLEN,
                "%02x:%02x:%02x:%02x:%02x:%02x",
                appData.deviceID[0], appData.deviceID[1],
                appData.deviceID[2], appData.deviceID[3],
                appData.deviceID[4], appData.deviceID[5]);

        /* Create HAP Engine */
        HAPEngineHandle = HAPEngine_create(accs, 2, deviceIDBuf, HAPEngine_CI_BRIDGE, &GetSetupCodeFxn, &params);
        LOG_ASSERT(HAPEngineHandle != NULL);

//        GPIO_write(Board_GPIO_LED2, Board_GPIO_LED_ON);

/*        Timer_Handle timer0;
        Timer_Params paramsT;

        Timer_Params_init(&paramsT);
        paramsT.period = 20000000;
        paramsT.periodUnits = Timer_PERIOD_US;
        paramsT.timerMode = Timer_CONTINUOUS_CALLBACK;
        paramsT.timerCallback = timerCallback;

        timer0 = Timer_open(Board_TIMER0, &paramsT);

        if (timer0 == NULL) {
            Display_printf(display, 0, 0, "(timer0 == NULL)");
        }

        if (Timer_start(timer0) == Timer_STATUS_ERROR)
            Display_printf(display, 0, 0, "(Timer_start(timer0) == Timer_STATUS_ERROR) ");*/


        /*
         * Run HAP server. This internally loops handling client requests
         * and may never return.
         */
        status = HAPEngine_serve(HAPEngineHandle, NULL, 0);
        LOG_ASSERT(status == HAPEngine_EOK);


        /* pend until HAPEngine_stop returns */
        sem_wait(&HAPStopSem);

        /* delete the HAPEngine */
        status = HAPEngine_delete(&HAPEngineHandle);
        LOG_ASSERT(status == HAPEngine_EOK);

        HAPEngine_reset();
        appInit(&appData, true);
        Log ("HomeKit Factory Reset. Accessory Name: %s", appData.accName);

    } while(1);
}
