
// Stores the global variables of a terminal driver.
typedef struct termAppGlobalFrame_t {
    int32_t width;
    int32_t height;
    // Pointer to runningApp_t.
    allocPointer_t observer;
    int32_t termInputIndex;
} termAppGlobalFrame_t;

#pragma pack(push, 1)

typedef struct testPacket_t {
    int8_t type;
    int32_t dataLength;
    // May be null if dataLength is zero.
    int8_t *data;
} testPacket_t;

typedef struct createFilePacketHeader_t {
    int8_t nameSize;
    int8_t type;
    int8_t isGuarded;
    int8_t hasAdminPerm;
} createFilePacketHeader_t;

#pragma pack(pop)


