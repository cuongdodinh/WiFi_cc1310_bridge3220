//26.08.17

#ifndef APP_SOCKETCLIENT_H_
#define APP_SOCKETCLIENT_H_

#include "simplelink.h"

extern _u16 ping;

bool SocketClientConnect ();
bool SocketClientPing();
void SocketClientDisconnect();
void SocketClientReconnect ();
bool SocketClientSendPacket (_u8 *socketSendBuff, int packetLen);
bool SocketClientSendLog(char* text, _u8 logPart, _u8 logType);
bool SocketClientSendSensorData (char* sensorMac, float sensorValue1, float sensorValue2);
void LogError (const char *pcFormat, ...);

#endif /* APP_SOCKETCLIENT_H_ */
