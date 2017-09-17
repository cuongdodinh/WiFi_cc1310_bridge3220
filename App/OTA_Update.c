//13.09.17

#include <stdio.h>
#include "global.h"
#include "OTA_Update.h"
#include "socketClient.h"
#include "socketProtocol.h"


static _u8 B64DecTable[256];
static _u8 B64EncTable[64];

static void B64_Init();
static _u8 *B64_Decode(const _u8 *pInputData, const int InputLength, _u8 *pOutputData, int *OutputLength);
static int16_t Ota_CloseAbort(int FileHandle);
static int16_t Ota_Rollback();

static int lFileHandle = -1;
static int filePos = 0;
static int fileCheckSum = 0;

//------------------------------------------------------------------------------------------------------------------------------------------------
bool OTA_Start (const char *fileName, _u32 fileMaxSize)
{
    UART_PRINT("fileName: %s \n\r", fileName);
    UART_PRINT("fileMaxSize: %d \n\r", fileMaxSize);

    uint32_t fsOpenFlags;
    uint32_t ulToken = 0;

    fsOpenFlags = SL_FS_CREATE;
    fsOpenFlags |= SL_FS_OVERWRITE;
    fsOpenFlags |= SL_FS_WRITE_BUNDLE_FILE;
    fsOpenFlags |= SL_FS_CREATE_FAILSAFE;
    fsOpenFlags |= SL_FS_CREATE_PUBLIC_WRITE;
    fsOpenFlags |= SL_FS_CREATE_SECURE;

    lFileHandle = sl_FsOpen((uint8_t *)fileName, fsOpenFlags| SL_FS_CREATE_MAX_SIZE(fileMaxSize),(_u32 *)&ulToken);

    if (lFileHandle < 0)
    {
        LOG_ERROR (lFileHandle);
        Ota_Rollback();
        return false;
    }

    filePos = 0;
    fileCheckSum = 0;

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
bool OTA_Continue (_u8 *fileBuff, _u16 partSize)
{
    if (lFileHandle < 0)
    {
        LOG_ERROR (lFileHandle);
        return false;
    }

//    UART_PRINT("filePos: %d   %d\n\r", filePos, partSize);

    int status = sl_FsWrite(lFileHandle, filePos, fileBuff, partSize);
    if (status < 0)
    {
        LOG_ERROR (status);
        Ota_CloseAbort(lFileHandle);
        Ota_Rollback();
        return false;
    }

    int s;
    for (s = 0; s < partSize; s++)
        fileCheckSum += fileBuff [s];

//    UART_PRINT("filePos: %d   %d       %lu\n\r", filePos, partSize, fileCheckSum);

    filePos += partSize;

    return true;
}


//------------------------------------------------------------------------------------------------------------------------------------------------
bool OTA_End (_u32 fileSize, _u64 fileCheckSumFromPacket, const char *certName, const _u8 *base64DecodeBuf, _u16 base64DecodeLen)
{
    UART_PRINT("fileSize: %d \n\r", fileSize);
    UART_PRINT("fileCheckSumFromPacket: %d \n\r", fileCheckSumFromPacket);
    UART_PRINT("fileCheckSum          : %d \n\r", fileCheckSum);
    UART_PRINT("certName: %s \n\r", certName);
    UART_PRINT("base64DecodeBuf: %d   %.*s \n\r", base64DecodeLen, base64DecodeLen, base64DecodeBuf);

//    int s;
//    for (s = 0; s < base64DecodeLen; s++)
//        UART_PRINT("signBuff: %d   %.*s \n\r", s, 1, base64DecodeBuf + s);

    if (fileSize != filePos)
    {
        LOG_ERROR (filePos);
        Ota_CloseAbort(lFileHandle);
        Ota_Rollback();
        return false;
    }

    if (fileCheckSum != fileCheckSumFromPacket)
    {
        LOG_ERROR (fileCheckSum);
        Ota_CloseAbort(lFileHandle);
        Ota_Rollback();
        return false;
    }

    char buff[100];
    sprintf (buff, "[OTA_End] Download success. Size: %d", filePos);
    SocketClientSendLog(buff, LogPart_ClientRuntime, LogType_None);

    _u8 signBuff[256];
    int signLen;

    B64_Init();
    if (B64_Decode(base64DecodeBuf, base64DecodeLen, signBuff, (int32_t *)&signLen) == NULL)
    {
        LOG_ERROR (0);
        Ota_CloseAbort(lFileHandle);
        Ota_Rollback();
        return false;
    }

//    UART_PRINT("signBuff: %d   %.*s \n\r", signLen, signLen, signBuff);

    int status = sl_FsClose(lFileHandle, (_u8*) certName, signBuff, signLen);

    if (status < 0)
    {
        LOG_ERROR (status);
        Ota_CloseAbort(lFileHandle);
        Ota_Rollback();
        return false;
    }
    else
        RebootMCU();

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
static void B64_Init()
{
    int8_t ch;
    int8_t i=0;

    /* build encode table */
    for (ch='A'; ch <= 'Z'; ch++) B64EncTable[i++] = ch;
    for (ch='a'; ch <= 'z'; ch++) B64EncTable[i++] = ch;
    for (ch='0'; ch <= '9'; ch++) B64EncTable[i++] = ch;
    B64EncTable[i++] = '+';
    B64EncTable[i++] = '/';

    /* build decode table */
    for (i=0; i<64; i++)
    {
        B64DecTable[(uint8_t) B64EncTable[i]] = i;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------------
static uint8_t *B64_Decode(const _u8 *pInputData, const int InputLength, _u8 *pOutputData, int *OutputLength)
{
    uint32_t decode_value;
    uint32_t nibble6_1, nibble6_2, nibble6_3, nibble6_4;
    int32_t i, j;

    if (InputLength % 4 != 0)
    {
        return NULL;
    }

    *OutputLength = InputLength / 4 * 3;
    if (pInputData[InputLength - 1] == '=') (*OutputLength)--;
    if (pInputData[InputLength - 2] == '=') (*OutputLength)--;

    if (pOutputData == NULL)
    {
        return NULL;
    }

    for (i = 0, j = 0; i < InputLength;)
    {
        nibble6_1 = pInputData[i] == '=' ? 0 & i++ : B64DecTable[pInputData[i++]];
        nibble6_2 = pInputData[i] == '=' ? 0 & i++ : B64DecTable[pInputData[i++]];
        nibble6_3 = pInputData[i] == '=' ? 0 & i++ : B64DecTable[pInputData[i++]];
        nibble6_4 = pInputData[i] == '=' ? 0 & i++ : B64DecTable[pInputData[i++]];

        decode_value = (nibble6_1 << 3 * 6) + (nibble6_2 << 2 * 6)  + (nibble6_3 << 1 * 6) + (nibble6_4 << 0 * 6);

        if (j < *OutputLength) pOutputData[j++] = (decode_value >> 2 * 8) & 0xFF;
        if (j < *OutputLength) pOutputData[j++] = (decode_value >> 1 * 8) & 0xFF;
        if (j < *OutputLength) pOutputData[j++] = (decode_value >> 0 * 8) & 0xFF;
    }

    return pOutputData;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
int16_t Ota_GetPendingCommitState()
{
    SlFsControlGetStorageInfoResponse_t SlFsControlGetStorageInfoResponse;
    int16_t Status;

    /* read bundle state and check if is "PENDING COMMIT" */
    Status = (int16_t)sl_FsCtl( ( SlFsCtl_e)SL_FS_CTL_GET_STORAGE_INFO, 0, NULL , NULL , 0, (uint8_t *)&SlFsControlGetStorageInfoResponse, sizeof(SlFsControlGetStorageInfoResponse_t), NULL );
    if (0 > Status)
    {
        LOG_ERROR (Status);
        return Status;
    }

    return (SL_FS_BUNDLE_STATE_PENDING_COMMIT == (SlFsBundleState_e)SlFsControlGetStorageInfoResponse.FilesUsage.Bundlestate);
}

//------------------------------------------------------------------------------------------------------------------------------------------------
int16_t Ota_Commit()
{
    SlFsControl_t FsControl;
    int16_t Status;

    FsControl.IncludeFilters = 0;
    Status = (int16_t)sl_FsCtl(SL_FS_CTL_BUNDLE_COMMIT, 0, NULL ,(uint8_t *)&FsControl, sizeof(SlFsControl_t), NULL, 0 , NULL);
    if( Status < 0)
        LOG_ERROR (Status);

    return Status;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
static int16_t Ota_CloseAbort(int FileHandle)
{
    _u8 abortSig = 'A';
    /* Close the file with signature 'A' which is ABORT */
    return sl_FsClose(FileHandle, 0, &abortSig, 1);
}

//------------------------------------------------------------------------------------------------------------------------------------------------
static int16_t Ota_Rollback()
{
    SlFsControl_t FsControl;
    int16_t Status;

    FsControl.IncludeFilters = 0;
    Status = (int16_t)sl_FsCtl(SL_FS_CTL_BUNDLE_ROLLBACK, 0, NULL ,(uint8_t *)&FsControl, sizeof(SlFsControl_t), NULL, 0 , NULL);
    if( 0 > Status )
        LOG_ERROR (Status);

    return Status;
}
