//26.08.17

#ifndef CC1310_INTERFACE_H_
#define CC1310_INTERFACE_H_

#include "simplelink.h"
#include <ti/drivers/UART.h>

#define UART_TASK_STACK_SIZE    1024
#define UART_TASK_PRIORITY      2

#define UART_PACKET_MAX_SIZE    128
#define UART_READ_BUFFER_SIZE   1024
#define UART_WRITE_BUFFER_SIZE  1512

#define UART_PACKET_LEN_OFFSET        0
#define UART_PACKET_SEQ_NUM_OFFSET    1
#define UART_PACKET_CRC_OFFSET        2
#define UART_PACKET_PAYLOAD_OFFSET    4
#define UART_PACKET_PACKET_TYPE_OFFSET    4

#define UART_PACKET_HEADER_SIZE      UART_PACKET_PAYLOAD_OFFSET
#define UART_PACKET_MAX_PAYLOAD_SIZE    UART_PACKET_MAX_SIZE - UART_PACKET_PAYLOAD_OFFSET


#define UART_EVENT_ALL                               0xFFFFFFFF
#define UART_EVENT_RX_DONE         (_u32)(1 << 0)


#define RESPONCE_TIMEOUT                        100 * 1000 / Clock_tickPeriod //in uSeconds
#define UART_MAX_HEADER_READ_COUNT                    1000 / RESPONCE_TIMEOUT //in uSeconds
#define UART_MAX_PACKET_READ_COUNT                    100 / RESPONCE_TIMEOUT //in uSeconds
#define UART_FLUSH_PERIOD                      100 * 1000 / Clock_tickPeriod //in uSeconds
#define UART_RECONNECT_PERIOD                  1000 * 1000 / Clock_tickPeriod //in uSeconds

#define UART_PACKET_TYPE_PACKET_FROM_NODE       1
#define UART_PACKET_TYPE_PACKET_TO_NODE         2
#define UART_PACKET_TYPE_PACKET_TO_NODE_ACK     3
#define UART_PACKET_TYPE_CMD                    4
#define UART_PACKET_TYPE_ALIVE                  5
#define UART_PACKET_TYPE_BRIDGE_ERROR           6


extern UART_Handle uart;

void UARTTaskInit ();
void* UARTTask (void *pvParameters);
void UART_AddToWriteBuffer (_u8* data, _u8 len);

#endif /* CC1310_INTERFACE_H_ */
