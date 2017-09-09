//26.08.17

#ifndef CC1310_INTERFACE_H_
#define CC1310_INTERFACE_H_

#include "simplelink.h"

#define MAX_MESSAGE_COUNT 100

typedef struct
{
    char  mac [8];
    _u8   messageIndex;
    char  message[10];
    _u16  radioIndex;
    _u8   UARTIndex;
}message_t;

extern message_t messageBuff [MAX_MESSAGE_COUNT];
extern _u8 messageCount;
extern _u8 messageIndex;

void UART_Task (void *pvParameters);

#endif /* CC1310_INTERFACE_H_ */
