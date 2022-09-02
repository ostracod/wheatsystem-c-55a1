
#pragma pack(push, 1)

// Stored at the beginning of each heap memory span.
typedef struct spanHeader_t {
    // Previous and next spans which are immediately adjacent.
    heapMemOffset_t previousByNeighbor;
    heapMemOffset_t nextByNeighbor;
    // Size of the span data region.
    heapMemOffset_t size;
    // Type of allocation stored in the span, if any.
    int8_t allocType;
} spanHeader_t;

// Stored after spanHeader_t when allocType is NONE_ALLOC_TYPE.
typedef struct emptySpanHeader_t {
    // Previous and next spans which have the same size degree.
    heapMemOffset_t previousByDegree;
    heapMemOffset_t nextByDegree;
    // Size degree of the span.
    int8_t degree;
} emptySpanHeader_t;

// Stored after spanHeader_t when allocType is not NONE_ALLOC_TYPE.
typedef struct allocHeader_t {
    // Previous and next allocations which have the same type.
    heapMemOffset_t previousByType;
    heapMemOffset_t nextByType;
    // Size of the allocation data region. Note that there may be unused space after the allocation within its parent span.
    heapMemOffset_t size;
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

// Stored at the beginning of a system sentry allocation.
typedef struct systemSentryHeader_t {
    int8_t type;
} systemSentryHeader_t;

// A sentry allocation which is created by the system. This struct is stored in the data region of a dynamicAlloc_t.
typedef struct systemSentry_t {
    systemSentryHeader_t header;
    int8_t data[0];
} systemSentry_t;

// Provides a mechanism for thread synchronization. Stored within the data region of a systemSentry_t.
typedef struct gateSentry_t {
    // Pointer to runningApp_t.
    allocPointer_t owner;
    int8_t mode;
    int8_t isOpen;
} gateSentry_t;

// Stored at the beginning of non-volatile storage
typedef struct storageHeader_t {
    storageOffset_t firstFileAddress;
} storageHeader_t;

// Stored at the beginning of each file.
typedef struct fileHeader_t {
    uint8_t attributes;
    uint8_t nameSize;
    storageOffset_t contentSize;
    storageOffset_t next;
} fileHeader_t;

// Represents a file which has been opened. Stored in the data region of a systemSentry_t.
typedef struct fileHandle_t {
    storageOffset_t address;
    uint8_t attributes;
    uint8_t nameSize;
    storageOffset_t contentSize;
    // Previous and next file handles in the linked list.
    allocPointer_t previous;
    allocPointer_t next;
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
    int8_t funcId;
    // Currently active localFrame_t.
    allocPointer_t localFrame;
    // Pointer to the gateSentry_t which is blocking execution.
    // Will be NULL_ALLOC_POINTER if no gate is blocking.
    allocPointer_t blockingGate;
} thread_t;

// Stored at the beginning of a running application allocation.
typedef struct runningAppHeader_t {
    // fileHandle_t of an application.
    allocPointer_t fileHandle;
    // The last action performed when attempting to kill the app.
    int8_t killAction;
    // The amount of time since the last kill action.
    int8_t killActionDelay;
    // Note that this member variable is only used by updateKillStates.
    // It is not updated regularly.
    heapMemOffset_t memUsage;
} runningAppHeader_t;

// Contains the state of a running application. This struct is stored in the data region of a heap allocation.
typedef struct runningApp_t {
    runningAppHeader_t header;
    // Stores global variables of the application.
    int8_t globalFrameData[0];
} runningApp_t;

typedef struct memUsageContext_t {
    allocPointer_t runningApp;
    heapMemOffset_t memUsage;
} memUsageContext_t;

// Stored at the beginning of a local frame.
typedef struct localFrameHeader_t {
    // thread_t in which the function is running.
    allocPointer_t thread;
    // runningApp_t which implements the function.
    allocPointer_t implementer;
    int32_t funcIndex;
    // Pointer to the previous localFrame_t.
    allocPointer_t previousLocalFrame;
    // argFrame_t to pass to the next function invocation.
    allocPointer_t nextArgFrame;
    // Last error code caught by an error handler.
    int8_t lastErrorCode;
    // Whether this local frame should receive throttleErr. If false, throttleErr will be changed to stateErr.
    int8_t shouldThrottle;
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
    int32_t funcTableLength;
    // Offset from the beginning of the file.
    int32_t appDataFilePos;
} bytecodeAppHeader_t;

// Stored in the function table of a bytecode application file.
typedef struct bytecodeFunc_t {
    int32_t funcId;
    int8_t isGuarded;
    int32_t argFrameSize;
    int32_t localFrameSize;
    // Offset from the beginning of the file.
    int32_t instructionBodyFilePos;
    int32_t instructionBodySize;
} bytecodeFunc_t;

// Stored at the beginning of the global frame of a bytecode application.
typedef struct bytecodeGlobalFrameHeader_t {
    int32_t funcTableLength;
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
            // Start address of the data region which contains the argument.
            heapMemOffset_t startAddress;
            // Start index of the argument within the parent data region.
            heapMemOffset_t index;
            // Size of the parent data region.
            heapMemOffset_t size;
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
typedef struct systemAppFunc_t {
    // ID of the function.
    int8_t id;
    int8_t isGuarded;
    int8_t argFrameSize;
    int8_t localFrameSize;
    // Task to perform whenever the function is scheduled.
    systemAppThreadAction_t threadAction;
} systemAppFunc_t;

// Defines a system application available on the current platform.
typedef struct systemApp_t {
    int8_t globalFrameSize;
    // Pointer to fixed values.
    const systemAppFunc_t *funcList;
    // Number of functions in funcList.
    int8_t funcAmount;
} systemApp_t;

#ifdef WHEATSYSTEM_UNIX
#pragma pack(push, 1)
#endif

// Stored at the beginning of the global frame of a system application.
typedef struct systemGlobalFrameHeader_t {
    // ID of the system application.
    int8_t id;
} systemGlobalFrameHeader_t;

// Stores the global variables of the terminal driver.
typedef struct termAppGlobalFrame_t {
    // Pointer to fileHandle_t.
    allocPointer_t observer;
    int32_t termInputIndex;
} termAppGlobalFrame_t;

#pragma pack(pop)


