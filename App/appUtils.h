//26.08.17

#ifndef APP_APPUTILS_H_
#define APP_APPUTILS_H_

extern char HexToChar (char b);

_u8 ToHexString (const _u8* byteArrayBuff, _u8 len, char* hexStringBuff);
void MakeCRC16Table(void);
_u16 GetCRC16(_u8 *buf, _u16 len);
void PrintBuffer (char* header, _u8 *buffToPrint, _u8 len);

void StopwatchRestart();
long StopwatchNanoseconds();
void StopwatchStoreInterval(int index);
char* StopwatchPrintStoredIntervals ();


#endif /* APP_APPUTILS_H_ */
