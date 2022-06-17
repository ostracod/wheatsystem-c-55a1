
#include "./headers.h"

allocPointer_t firstAlloc = NULL_ALLOC_POINTER;
heapMemoryOffset_t heapMemorySizeLeft = HEAP_MEMORY_SIZE;
allocPointer_t firstThread = NULL_ALLOC_POINTER;
int16_t killStatesDelay = 0;

int8_t unhandledErrorCode = 0;

allocPointer_t nextThread;
allocPointer_t currentThread;
allocPointer_t currentLocalFrame;
allocPointer_t currentImplementer;
allocPointer_t currentImplementerFileHandle;
int8_t currentImplementerFileType;

instructionArg_t instructionArgArray[MAXIMUM_ARG_AMOUNT];
int32_t currentInstructionFilePos;
int32_t instructionBodyStartFilePos;
int32_t instructionBodyEndFilePos;


