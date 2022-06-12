
// First allocation in the linked list.
allocPointer_t firstAlloc;
// The amount of unused space remaining in heap memory.
heapMemoryOffset_t heapMemorySizeLeft;
// The amount of time since the resource system last managed memory usage.
int16_t memoryManagementDelay;

// Stores the last thrown error code. The value of this variable must be checked after invoking certain functions.
int8_t unhandledErrorCode;

// Stores the runningApp_t of the currently scheduled thread.
allocPointer_t currentThreadApp;
// Stores the active localFrame_t of the currently scheduled function invocation.
allocPointer_t currentLocalFrame;
// Stores the runningApp_t which implements the currently scheduled function invocation.
allocPointer_t currentImplementer;
// Stores the fileHandle_t of the application which implements the currently scheduled function invocation.
allocPointer_t currentImplementerFileHandle;
int8_t currentImplementerFileType;

// Stores arguments of the bytecode instruction which is currently being interpreted.
instructionArg_t instructionArgArray[MAXIMUM_ARG_AMOUNT];
// Describes the location of the current bytecode instruction within its application file.
int32_t currentInstructionFilePos;
int32_t instructionBodyStartFilePos;
int32_t instructionBodyEndFilePos;


