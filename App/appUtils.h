//26.08.17

#ifndef APP_APPUTILS_H_
#define APP_APPUTILS_H_

extern char HexToChar (char b);
extern _u8 ToHexString (_u8* byteArrayBuff, _u8 len, char* hexStringBuff);
extern void MakeCRC16Table(void);
extern _u16 GetCRC16(_u8 *buf, _u16 len);

#endif /* APP_APPUTILS_H_ */
