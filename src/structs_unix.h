
#pragma pack(push, 1)

// Represents a file which has been opened. Stored in the data region of a dynamicHeapAlloc_t.
typedef struct fileHandle_t {
    int8_t *name;
    int8_t *unixPath;
    int8_t hasAdminPerm;
    int8_t isGuarded;
    int8_t type;
    int32_t contentSize;
    int8_t contentIsDirty;
    int8_t *content;
    // Pointer to runningApp_t.
    allocPointer_t runningApp;
    int8_t initErr;
    int8_t openDepth;
} fileHandle_t;

// Stores the global variables of a terminal driver.
typedef struct termAppGlobalFrame_t {
    int32_t width;
    int32_t height;
    // Pointer to runningApp_t.
    allocPointer_t observer;
    int32_t termInputIndex;
} termAppGlobalFrame_t;

#pragma pack(pop)


