
// Stores the first empty span of each size degree.
heapMemOffset_t emptySpansByDegree[SPAN_DEGREE_AMOUNT];
// Stores one bit for every span size degree. Each bit equal to 1 indicates the existence of an empty span with the corresponding degree.
uint8_t emptySpanBitField[SPAN_BIT_FIELD_SIZE];
// Stores one bit for every possible span position. Each bit equal to 1 indicates the start of a span which contains an allocation.
uint8_t allocBitField[ALLOC_BIT_FIELD_SIZE];
// The amount of unused space remaining in heap memory.
heapMemOffset_t heapMemSizeLeft;

// First thread_t in the linked list.
allocPointer_t firstThread;
// First runningApp_t in the linked list.
allocPointer_t firstRunningApp;
// The amount of time since app kill states were last updated.
int16_t killStatesDelay;
// Stores the last thrown error code. The value of this variable must be checked after invoking certain functions.
int8_t unhandledErrorCode;

// Stores the next thread_t to schedule.
allocPointer_t nextThread;
// Stores the currently scheduled thread_t.
allocPointer_t currentThread;
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


