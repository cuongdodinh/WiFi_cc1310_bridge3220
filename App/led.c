/*
 * Copyright (c) 2014-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== led.c ========
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <ti/drivers/GPIO.h>
#include <ti/hap/HAPAcc.h>

/* Board Header Files */
#include "Board.h"

#include "led.h"

#define LED_COUNT 1

static HAPEngine_SrvStatus setPower(unsigned int aId,
    HAPEngine_CDescPtr cDesc, bool value);
static HAPEngine_SrvStatus getPower(unsigned int aId,
    HAPEngine_CDescPtr cDesc, bool *value);
static HAPEngine_SrvStatus getName(unsigned int aId,
    HAPEngine_CDescPtr cDesc, int size, char *value);

#define MAXNAME 32

const HAPEngine_CharacteristicDesc ledCharDescs[] = {
    { HAPEngine_C_ON, LED_ON_IID,
        HAPEngine_CPERM_R | HAPEngine_CPERM_W | HAPEngine_CPERM_E |
        HAPEngine_CPERM_NC_E, HAPEngine_CFORMAT_BOOL, HAPEngine_CCS_NONE, "on",
        { .boolFxns = { setPower, getPower } }, HAPEngine_CUNIT_UNITLESS },
    { HAPEngine_C_NAME, LED_NAME_IID, HAPEngine_CPERM_R,
        HAPEngine_CFORMAT_STRING, HAPEngine_CCS_STRING(MAXNAME), "name",
        { .stringFxns = { NULL, getName } }, HAPEngine_CUNIT_UNITLESS }
};

const HAPEngine_ServiceDesc ledServiceDesc = {
    HAPEngine_S_LIGHTBULB,
    LED_SERVICE_IID,
    true,   /* enable service-wide lock */
    HAPEngine_SPROP_PRIMARY, LED_SERVICE_SIG_IID,
    HAPEngine_SLINKED_NONE, NULL,
    sizeof(ledCharDescs)/sizeof(ledCharDescs[0]),
    (HAPEngine_CharacteristicDesc *)ledCharDescs
};

typedef struct led_Object {
    char curName[MAXNAME];
    bool curPower;
} led_Object;

static led_Object led_instance = {0};

void LED_identifyRoutine(void)
{

}

void LED_init(void)
{
    led_instance.curPower = false;
    strncpy(led_instance.curName, "led", MAXNAME);
    GPIO_write(Board_GPIO_LED2, Board_GPIO_LED_OFF);
}

void LED_restoreState(void)
{
    if (led_instance.curPower == true) {
        GPIO_write(Board_GPIO_LED2, Board_GPIO_LED_ON);
    }
    else {
        GPIO_write(Board_GPIO_LED2, Board_GPIO_LED_OFF);
    }
}
#include <ti/hap/ip/HAPEngine.h>
//#include "leaksensor.h"
extern HAPEngine_Handle engineHandle;

static HAPEngine_SrvStatus setPower(unsigned int aId,
    HAPEngine_CDescPtr cDesc, bool value)
{
    bool curPower;
    if (GPIO_read(Board_GPIO_LED2)) {
        curPower = true;
    }
    else{
        curPower = false;
    }

    HAPEngine_SrvStatus result = (value == curPower) ?
        HAPEngine_SrvStatus_EOKNOCHANGE : HAPEngine_SrvStatus_EOK;

    if ((value == true) && (result != HAPEngine_SrvStatus_EOKNOCHANGE)) {
        GPIO_write(Board_GPIO_LED2, Board_GPIO_LED_ON);
    }
    else if (result != HAPEngine_SrvStatus_EOKNOCHANGE) {
        GPIO_write(Board_GPIO_LED2, Board_GPIO_LED_OFF);
    }
    led_instance.curPower = value;

//    if (curPower != value)
    {

//        GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
//        HAPEngine_updateCharacteristic(engineHandle, 2, LEAK_LEAKDETECTED_IID);

    }


    return (result);
}

static HAPEngine_SrvStatus getPower(unsigned int aId,
    HAPEngine_CDescPtr cDesc, bool *value)
{
    if (GPIO_read(Board_GPIO_LED2)) {
        *value = true;
    }
    else {
        *value = false;
    }

    return (HAPEngine_SrvStatus_EOK);
}

static HAPEngine_SrvStatus getName(unsigned int aId,
    HAPEngine_CDescPtr cDesc, int size, char *value)
{
    strncpy(value, led_instance.curName, size);

    return (HAPEngine_SrvStatus_EOK);
}
