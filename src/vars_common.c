
#include "./headers.h"

allocPointer_t firstAlloc = NULL_ALLOC_POINTER;
heapMemoryOffset_t heapMemorySizeLeft = HEAP_MEMORY_SIZE;
int16_t killStatesDelay = 0;

int8_t unhandledErrorCode = 0;

allocPointer_t currentThreadApp;
allocPointer_t currentLocalFrame;
allocPointer_t currentImplementer;
allocPointer_t currentImplementerFileHandle;
int8_t currentImplementerFileType;

instructionArg_t instructionArgArray[MAXIMUM_ARG_AMOUNT];
int32_t currentInstructionFilePos;
int32_t instructionBodyStartFilePos;
int32_t instructionBodyEndFilePos;


