// simplelink includes
//#include "device.h"

#include <stdint.h>

/* POSIX Header files */
#include <pthread.h>

/* RTOS header files */
#include <ti/sysbios/BIOS.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* Example/Board Header files */
#include "Board.h"


// driverlib includes
#include "hw_memmap.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include "timer.h"
#include "utils.h"
#include "gpio.h"
#include "prcm.h"

#include "uart.h"
#include "uart_term.h"

#include "App/global.h"
#include "App/cc1310_interface.h"
#include "App/OTA_Update.h"
#include <App/appUtils.h>
#include "App/WiFi.h"
#include "App/HAPServer.h"
#include "App/socketClient.h"


#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/Timer.h>
#include "platform.h"


#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif

#define OSI_STACK_SIZE          3000

unsigned long g_ulTimerInts;   //  Variable used in Timer Interrupt Handler
char g_BlinkCounter = 0;
_u8 macAddressVal[6];
sem_t HAPInitSem, HAPStopSem, WiFiConnectSem, btnSem;


e_AppStatusCodes g_appState = DEVICE_STARTED;


//--------------------------------------------------------------------------------------------------------------------------
void BlinkIntHandler(void)
{
/*    unsigned long ulInts;

    //
    // Clear all pending interrupts from the timer we are
    // currently using.
    //
    ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
    MAP_TimerIntClear(TIMERA0_BASE, ulInts);

    if (g_BlinkCounter <= 0)
        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    else
        g_BlinkCounter--;*/
}

//----------------------------------------------------------------------------------------------------------------------------------
void BlinkLED(char period)
{
//    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    g_BlinkCounter = period;
}

//*****************************************************************************
//
//! Periodic Timer Interrupt Handler
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void
TimerPeriodicIntHandler(void)
{
    unsigned long ulInts;

    //
    // Clear all pending interrupts from the timer we are
    // currently using.
    //
    ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
    MAP_TimerIntClear(TIMERA0_BASE, ulInts);

    //
    // Increment our interrupt counter.
    //
    g_ulTimerInts++;
    if(!(g_ulTimerInts & 0x1))
    {
        //
        // Off Led
        //
//        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    }
    else
    {
        //
        // On Led
        //
 //       GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    }
}


bool sendInProgress = false;



//----------------------------------------------------------------------------------------------------------------------------------
void BlinkLed_Task (void *pvParameters)
{
/*    int rebootPeriodSec = 3 * 60;

    while (1)
    {
        osi_Sleep(500);
//        GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

        osi_Sleep(500);
//        GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

        rebootPeriodSec--;

        if (rebootPeriodSec == 0)
            RebootMCU();
    }
*/

}

#define SPAWN_TASK_PRIORITY     (9)
#define TASK_STACK_SIZE         (3*1024)

pthread_t g_spawn_thread = (pthread_t)NULL;
pthread_t g_WiFi_thread = (pthread_t)NULL;
pthread_t g_UART_thread = (pthread_t)NULL;

/*!
 *  \brief  Application's events
 */
typedef enum
{
    APP_EVENT_NULL,
    APP_EVENT_STARTED,
    APP_EVENT_CONNECTED,
    APP_EVENT_IP_ACQUIRED,
    APP_EVENT_DISCONNECT,  /* used also for IP lost */
    APP_EVENT_PROVISIONING_STARTED,
    APP_EVENT_PROVISIONING_SUCCESS,
    APP_EVENT_PROVISIONING_STOPPED,
    APP_EVENT_PING_COMPLETE,
    APP_EVENT_OTA_START,
    APP_EVENT_CONTINUE,
    APP_EVENT_OTA_CHECK_DONE,
    APP_EVENT_OTA_DOWNLOAD_DONE,
    APP_EVENT_OTA_ERROR,
    APP_EVENT_TIMEOUT,
    APP_EVENT_ERROR,
    APP_EVENT_RESTART,
    APP_EVENT_MAX

}e_AppEvent;

SlNetAppEvent_t g_lastSlEvent;



/*!
 *  \brief      The interrupt handler for the async-evt timer
 *  \param      None
 * \return     None
 */
void AsyncEvtTimerIntHandler(sigval val)
{
#ifdef USE_TIRTOS
    s_AppContext *const pCtx = &gAppCtx;
#endif
    Platform_TimerInterruptClear();
    /* One Shot timer */
//    StopAsyncEvtTimer();
//    SignalEvent(APP_EVENT_TIMEOUT);
}

#define DEBOUNCETIME 200 /* milliseconds */
uint32_t prevTicks = 0xFFFF;
uint32_t curTicks, msPeriod;

//-------------------------------------------------------------------------------------------------------------------------------------
void gpioButtonFxn0(uint_least8_t index)
{
    curTicks = ClockP_getSystemTicks();
    msPeriod = ClockP_getSystemTickPeriod() / 1000;

    if ((curTicks - prevTicks) >= (DEBOUNCETIME/msPeriod)) {
        if (HAPEngineHandle) {
            sem_post(&btnSem);
        }
    }
    prevTicks = curTicks;
    GPIO_clearInt(Board_BUTTON0);
}

int timeSinceStartup = 0; //in seconds

//-------------------------------------------------------------------------------------------------------------------------------------
void timerCallback(Timer_Handle myHandle)
{
    timeSinceStartup += 5;
}

//-------------------------------------------------------------------------------------------------------------------------------------
void * mainThread( void *pvParameters )
{
    uint32_t RetVal ;
    pthread_attr_t      pAttrs;
    pthread_attr_t      pAttrs_spawn;
    pthread_t HAPServerThread;
    struct sched_param  priParam;
    struct timespec     ts = {0};

    Board_initGPIO();
    SPI_init();
    I2C_init();
    Timer_init();


 /* init the platform code..*/
    Platform_Init();
    Platform_TimerInit(&AsyncEvtTimerIntHandler);

    ///Platform_TimeoutStart(&pCtx->PlatformTimeout_Led, LED_TOGGLE_TIMEOUT);

    GPIO_write(Board_LED0, Board_LED_OFF);
    GPIO_write(Board_LED1, Board_LED_OFF);
    GPIO_write(Board_LED2, Board_LED_OFF);

    GPIO_setCallback(Board_BUTTON0, gpioButtonFxn0);

    sem_init(&HAPInitSem, 0, 0);
    sem_init(&HAPStopSem, 0, 0);
    sem_init(&WiFiConnectSem, 0, 0);
    sem_init(&btnSem, 0, 0);

    UARTTaskInit ();

    SocketClientInit ();

    /* init Terminal, and print App name */
    InitTerm();
    /* initilize the realtime clock */

    clock_settime(CLOCK_REALTIME, &ts);

    UART_PRINT("WiFiCam v. %d \n\r", APPLICATION_VERSION);

    MakeCRC16Table();


    //create the sl_Task
    pthread_attr_init(&pAttrs_spawn);
    priParam.sched_priority = SPAWN_TASK_PRIORITY;
    RetVal = pthread_attr_setschedparam(&pAttrs_spawn, &priParam);
    RetVal |= pthread_attr_setstacksize(&pAttrs_spawn, TASK_STACK_SIZE);
    RetVal |= pthread_create(&g_spawn_thread, &pAttrs_spawn, sl_Task, NULL);

    LOG_NON_ZERO (RetVal);

    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 1;
    RetVal = pthread_attr_setschedparam(&pAttrs, &priParam);
    RetVal |= pthread_attr_setstacksize(&pAttrs, TASK_STACK_SIZE);
    RetVal |= pthread_create(&g_WiFi_thread, &pAttrs, WiFi_Task, NULL);

    LOG_NON_ZERO (RetVal);

    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 3;
    RetVal = pthread_attr_setschedparam(&pAttrs, &priParam);
    RetVal |= pthread_attr_setstacksize(&pAttrs, TASK_STACK_SIZE);
    RetVal |= pthread_create(&g_UART_thread, &pAttrs, UARTTask, NULL);

    LOG_NON_ZERO (RetVal);

    /* Create thread for HAP server */
    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 2;
    RetVal = pthread_attr_setschedparam(&pAttrs, &priParam);
    RetVal |= pthread_attr_setstacksize(&pAttrs, (5 * 1024));
    RetVal |= pthread_create(&HAPServerThread, &pAttrs, HAPServer_Task, NULL);

    LOG_NON_ZERO (RetVal);

    Timer_Handle timer0;
    Timer_Params paramsT;

    Timer_Params_init(&paramsT);
    paramsT.period = 5000000;
    paramsT.periodUnits = Timer_PERIOD_US;
    paramsT.timerMode = Timer_CONTINUOUS_CALLBACK;
    paramsT.timerCallback = timerCallback;

    timer0 = Timer_open(Board_TIMER0, &paramsT);

    if (timer0 == NULL)
        LOG_ERROR (timer0);

    if (Timer_start(timer0) == Timer_STATUS_ERROR)
        LOG_ERROR (timer0);

    return(0);
}

/* Stack size in bytes */
#define THREADSTACKSIZE    4096

//--------------------------------------------------------------------------------------------------------------------------------------------
void main(void) {
    pthread_t           thread;
    pthread_attr_t      pAttrs;
    struct sched_param  priParam;
    int                 retc;
    int                 detachState;

    /* Call board init functions */
    Board_initGeneral();

    /* Set priority and stack size attributes */
    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 1;

    detachState = PTHREAD_CREATE_DETACHED;
    retc = pthread_attr_setdetachstate(&pAttrs, detachState);
    if (retc != 0) {
        /* pthread_attr_setdetachstate() failed */
        while (1);
    }

    pthread_attr_setschedparam(&pAttrs, &priParam);

    retc |= pthread_attr_setstacksize(&pAttrs, THREADSTACKSIZE);
    if (retc != 0) {
        /* pthread_attr_setstacksize() failed */
        while (1);
    }

    retc = pthread_create(&thread, &pAttrs, mainThread, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        while (1);
    }

    BIOS_start();
}
