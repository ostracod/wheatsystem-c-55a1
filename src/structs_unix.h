
typedef struct testPacket_t {
    int8_t type;
    int32_t dataLength;
    // May be null if dataLength is zero.
    int8_t *data;
} testPacket_t;

#pragma pack(push, 1)

typedef struct createFilePacketHeader_t {
    int8_t nameSize;
    int8_t type;
    int8_t isGuarded;
    int8_t hasAdminPerm;
} createFilePacketHeader_t;

typedef struct createdAllocPacketBody_t {
    int32_t pointer;
    int32_t startAddress;
    int32_t endAddress;
} createdAllocPacketBody_t;

#pragma pack(pop)


