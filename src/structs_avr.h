
#pragma pack(push, 1)

// Stores the global variables of the terminal driver.
typedef struct termAppGlobalFrame_t {
    // Pointer to fileHandle_t.
    allocPointer_t observer;
    int32_t termInputIndex;
} termAppGlobalFrame_t;

// Stores the global variables of the serial driver.
typedef struct serialAppGlobalFrame_t {
    // Pointer to fileHandle_t.
    allocPointer_t observer;
    int32_t serialInputIndex;
} serialAppGlobalFrame_t;

#pragma pack(pop)


