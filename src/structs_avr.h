
#pragma pack(push, 1)

// Stores the global variables of a terminal driver.
typedef struct termAppGlobalFrame_t {
    // Pointer to fileHandle_t.
    allocPointer_t observer;
    int32_t termInputIndex;
} termAppGlobalFrame_t;

#pragma pack(pop)


