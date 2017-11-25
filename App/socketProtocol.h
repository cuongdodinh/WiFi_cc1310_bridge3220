//26.08.17

#ifndef APP_SOCKETPROTOCOL_H_
#define APP_SOCKETPROTOCOL_H_

enum HeaderOffsets {
    HeaderOffsetsPacketSize = 0,
    HeaderOffsetsCheckSum = 2,
    HeaderOffsetsFirstEnd = 4,
    HeaderOffsetsMac = 4,
    HeaderOffsetsPacketNum = 10,
    HeaderOffsetsSecondEnd = 11
};

enum PacketTypes {
    PType_Auth = 1,
    PType_Ping = 2,
    PType_SensorData = 3,
    PType_File = 4,
    PType_Log = 5,
    PType_NodePacket = 6
};

enum PacketSubTypes {
    PSubType_FileStart = 1,
    PSubType_FileContinue = 2,
    PSubType_FileEnd = 3
};

enum LogTypes {
    LogType_None = 0,
    LogType_Error = 1,
    LogType_CriticalError = 2,
    LogType_Mark1 = 3,
    LogType_Mark2 = 4,
    LogType_HTTPGetHeader = 5
};

enum LogParts {
    LogPart_Packets = 1,
    LogPart_ServerRuntime = 2,
    LogPart_ClientRuntime = 3
};

#endif /* APP_SOCKETPROTOCOL_H_ */
