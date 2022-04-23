
#pragma pack(push, 1)

// Stored at the beginning of each file.
typedef struct fileHeader_t {
    uint8_t attributes;
    uint8_t nameSize;
    int32_t contentSize;
    int32_t next;
} fileHeader_t;

// Represents a file which has been opened. Stored in the data region of a dynamicHeapAlloc_t.
typedef struct fileHandle_t {
    int32_t address;
    uint8_t attributes;
    uint8_t nameSize;
    int32_t contentSize;
    // Pointer to runningApp_t.
    allocPointer_t runningApp;
    int8_t initErr;
    int8_t openDepth;
} fileHandle_t;

// Stores the global variables of a terminal driver.
typedef struct termAppGlobalFrame_t {
    // Pointer to runningApp_t.
    allocPointer_t observer;
} termAppGlobalFrame_t;

#pragma pack(pop)


