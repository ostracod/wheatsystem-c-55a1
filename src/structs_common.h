
#pragma pack(push, 1)

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
    // runningApp_t which created the allocation.
    allocPointer_t creator;
} dynamicAllocHeader_t;

// A heap allocation which is directly accessible by bytecode apps. This struct is stored in the data region of a heap allocation.
typedef struct dynamicAlloc_t {
    dynamicAllocHeader_t header;
    // Data region of the dynamic allocation.
    int8_t data[0];
} dynamicAlloc_t;

// Stored at the beginning of non-volatile storage
typedef struct storageSpaceHeader_t {
    int32_t firstFileAddress;
} storageSpaceHeader_t;

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

// Represents a thread spawned from a running application.
typedef struct thread_t {
    // runningApp_t which implements the base function in the thread.
    allocPointer_t runningApp;
    // ID of the base function in the thread.
    int8_t functionId;
    // Currently active localFrame_t.
    allocPointer_t localFrame;
    // Previous thread_t in the linked list.
    allocPointer_t previous;
    // Next thread_t in the linked list.
    allocPointer_t next;
} thread_t;

// Stored at the beginning of a running application allocation.
typedef struct runningAppHeader_t {
    // fileHandle_t of an application.
    allocPointer_t fileHandle;
    // Whether the application is blocked by a "wait" instruction.
    int8_t isWaiting;
    // The last action performed when attempting to kill the app.
    int8_t killAction;
    // The amount of time since the last kill action.
    int8_t killActionDelay;
    // Note that this member variable is only used by updateKillStates.
    // It is not updated regularly.
    heapMemoryOffset_t memoryUsage;
    // Previous runningApp_t in the linked list.
    allocPointer_t previous;
    // Next runningApp_t in the linked list.
    allocPointer_t next;
} runningAppHeader_t;

// Contains the state of a running application. This struct is stored in the data region of a heap allocation.
typedef struct runningApp_t {
    runningAppHeader_t header;
    // Stores global variables of the application.
    int8_t globalFrameData[0];
} runningApp_t;

// Stored at the beginning of a local frame.
typedef struct localFrameHeader_t {
    // thread_t in which the function is running.
    allocPointer_t thread;
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

// Stored at the beginning of an argument frame.
typedef struct argFrameHeader_t {
    // thread_t to which the argument frame belongs.
    allocPointer_t thread;
} argFrameHeader_t;

// Contains variables passed from one function invocation to another. This struct is stored in the data region of a heap allocation.
typedef struct argFrame_t {
    argFrameHeader_t header;
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

typedef struct instructionArg_t {
    // The prefix reference type can only be HEAP_MEM_REF_TYPE,
    // CONSTANT_REF_TYPE, or APP_DATA_REF_TYPE.
    uint8_t prefix;
    union {
        // For HEAP_MEM_REF_TYPE, the union contains startAddress, size, and index.
        // For CONSTANT_REF_TYPE, the union contains constantValue.
        // For APP_DATA_REF_TYPE, the union contains appDataIndex.
        struct {
            heapMemoryOffset_t startAddress;
            heapMemoryOffset_t index;
            heapMemoryOffset_t size;
        };
        int32_t constantValue;
        int32_t appDataIndex;
    };
} instructionArg_t;

// The structs here cannot be packed, or else we get a
// compilation error under macOS arm64.
#ifdef WHEATSYSTEM_UNIX
#pragma pack(pop)
#endif

// Defines a function available in a system application.
typedef struct systemAppFunction_t {
    // ID of the function.
    int8_t id;
    int8_t isGuarded;
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

#ifdef WHEATSYSTEM_UNIX
#pragma pack(push, 1)
#endif

// Stored at the beginning of the global frame of a system application.
typedef struct systemGlobalFrameHeader_t {
    // ID of the system application.
    int8_t id;
} systemGlobalFrameHeader_t;

#pragma pack(pop)


