
// Stores the global variables of a terminal driver.
typedef struct termAppGlobalFrame_t {
    int32_t width;
    int32_t height;
    // Pointer to runningApp_t.
    allocPointer_t observer;
    int32_t termInputIndex;
} termAppGlobalFrame_t;


