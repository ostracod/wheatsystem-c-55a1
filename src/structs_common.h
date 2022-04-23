
#ifdef WHEATSYSTEM_AVR
#pragma pack(push, 1)
#endif

// Stored at the beginning of each heap allocation.
typedef struct allocHeader_t {
    int8_t type;
    heapMemoryOffset_t size;
    allocPointer_t next;
} allocHeader_t;

// Stored at the beginning of a dynamic allocation.
typedef struct dynamicAllocHeader_t {
    // Guard and sentry flags.
    int8_t attributes;
    // fileHandle_t of app which created the allocation.
    allocPointer_t creator;
} dynamicAllocHeader_t;

// A heap allocation which is directly accessible by bytecode apps. This struct is stored in the data region of a heap allocation.
typedef struct dynamicAlloc_t {
    dynamicAllocHeader_t header;
    // Data region of the dynamic allocation.
    int8_t data[0];
} dynamicAlloc_t;

// Stored at the beginning of a running application allocation.
typedef struct runningAppHeader_t {
    // fileHandle_t of an application.
    allocPointer_t fileHandle;
    // Currently active localFrame_t.
    allocPointer_t localFrame;
    // Whether the application is blocked by a "wait" instruction.
    int8_t isWaiting;
} runningAppHeader_t;

// Contains the state of a running application. This struct is stored in the data region of a heap allocation.
typedef struct runningApp_t {
    runningAppHeader_t header;
    // Stores global variables of the application.
    int8_t globalFrameData[0];
} runningApp_t;

// Stored at the beginning of a local frame.
typedef struct localFrameHeader_t {
    // runningApp_t which implements the function.
    allocPointer_t implementer;
    int32_t functionIndex;
    // Pointer to the previous localFrame_t.
    allocPointer_t previousLocalFrame;
    // argFrame_t to pass to the next function invocation.
    allocPointer_t nextArgFrame;
    // Last error code caught by an error handler.
    int8_t lastErrorCode;
} localFrameHeader_t;

// Contains variables used by a single function invocation. This struct is stored in the data region of a heap allocation.
typedef struct localFrame_t {
    localFrameHeader_t header;
    int8_t data[0];
} localFrame_t;

// Contains variables passed from one function invocation to another. This struct is stored in the data region of a heap allocation.
typedef struct argFrame_t {
    int8_t data[0];
} argFrame_t;

// Stored at the beginning of a bytecode application file.
typedef struct bytecodeAppHeader_t {
    int32_t globalFrameSize;
    // Number of functions in the function table.
    int32_t functionTableLength;
    // Offset from the beginning of the file.
    int32_t appDataFilePos;
} bytecodeAppHeader_t;

// Stored in the function table of a bytecode application file.
typedef struct bytecodeFunction_t {
    int32_t functionId;
    int8_t isGuarded;
    int32_t argFrameSize;
    int32_t localFrameSize;
    // Offset from the beginning of the file.
    int32_t instructionBodyFilePos;
    int32_t instructionBodySize;
} bytecodeFunction_t;

// Stored at the beginning of the global frame of a bytecode application.
typedef struct bytecodeGlobalFrameHeader_t {
    int32_t functionTableLength;
    int32_t appDataFilePos;
    int32_t appDataSize;
} bytecodeGlobalFrameHeader_t;

// Stored at the beginning of each local frame of a bytecode application.
typedef struct bytecodeLocalFrameHeader_t {
    int32_t instructionBodyStartFilePos;
    int32_t instructionBodyEndFilePos;
    // Position of current instruction to interpret.
    int32_t instructionFilePos;
    // Offset from the beginning of the current function body.
    int32_t errorHandler;
} bytecodeLocalFrameHeader_t;

// Defines a function available in a system application.
typedef struct systemAppFunction_t {
    // ID of the function.
    int8_t id;
    int8_t argFrameSize;
    int8_t localFrameSize;
    // Task to perform whenever the function is scheduled.
    systemAppThreadAction_t threadAction;
} systemAppFunction_t;

// Defines a system application available on the current platform.
typedef struct systemApp_t {
    int8_t globalFrameSize;
    // Pointer to fixed values.
    const systemAppFunction_t *functionList;
    // Number of functions in functionList.
    int8_t functionAmount;
} systemApp_t;

// Stored at the beginning of the global frame of a system application.
typedef struct systemGlobalFrameHeader_t {
    // ID of the system application.
    int8_t id;
} systemGlobalFrameHeader_t;

#ifdef WHEATSYSTEM_AVR
#pragma pack(pop)
#endif


