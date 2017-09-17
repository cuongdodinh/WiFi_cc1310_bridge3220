//26.08.17

#ifndef OTA_UPDATE_H_
#define OTA_UPDATE_H_

bool OTA_Start (const char *fileName, _u32 fileMaxSize);
bool OTA_Continue (_u8 *fileBuff, _u16 partSize);
bool OTA_End (_u32 fileSize, _u64 fileCheckSumFromPacket, const char *certName, const _u8 *base64DecodeBuf, _u16 base64DecodeLen);
int16_t Ota_GetPendingCommitState();
int16_t Ota_Commit();


#endif /* OTA_UPDATE_H_ */
