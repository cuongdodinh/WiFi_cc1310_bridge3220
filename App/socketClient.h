//26.08.17

#ifndef APP_SOCKETCLIENT_H_
#define APP_SOCKETCLIENT_H_

#include "simplelink.h"


#define SocketClient_AddToSendBuffer_OverlfowErrorPacketText    "SocketClient_AddToSendBuffer_OverlfowError"
#define SocketClient_AddToSendBuffer_OverlfowErrorPacketSize    sizeof (SocketClient_AddToSendBuffer_OverlfowErrorPacketText) + 4
#define SOCKET_SEND_QUEUE_BUFF_SIZE                             3000

extern _u16 g_ping;

void SocketClientInit ();

int SocketClientProcessRecv ();
bool SocketClientProcessSend ();

bool SocketClientConnect ();
bool SocketClientPing();
void SocketClientDisconnect();
void SocketClientReconnect ();
bool SocketClientSendPacket (_u8 *socketSendBuff, int packetLen);
bool SocketClientSendLog(char* text, _u8 logPart, _u8 logType);
bool SocketClientSendSensorData (char* sensorMac, float sensorValue1, float sensorValue2);
bool SocketClientSendNodePacket(_u8* data, _u8 len);

void Log (const char *pcFormat, ...);

#endif /* APP_SOCKETCLIENT_H_ */
