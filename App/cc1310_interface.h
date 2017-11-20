//26.08.17

#ifndef CC1310_INTERFACE_H_
#define CC1310_INTERFACE_H_

#include "simplelink.h"
#include <ti/drivers/UART.h>

extern UART_Handle uart;

void* UARTTask (void *pvParameters);
void UART_AddToWriteBuffer (_u8* data, _u8 len);

#endif /* CC1310_INTERFACE_H_ */
