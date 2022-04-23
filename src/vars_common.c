
#include "./headers.h"

int8_t heapMemory[HEAP_MEMORY_SIZE];
allocPointer_t firstAlloc = NULL_ALLOC_POINTER;

int8_t unhandledErrorCode = 0;

allocPointer_t currentThreadApp;
allocPointer_t currentLocalFrame;
allocPointer_t currentImplementer;
allocPointer_t currentImplementerFileHandle;
int8_t currentImplementerFileType;


