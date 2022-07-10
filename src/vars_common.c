
#include "./headers.h"

heapMemOffset_t heapMemSizeLeft;
allocPointer_t firstThread;
allocPointer_t firstRunningApp;
int16_t killStatesDelay;
int8_t unhandledErrorCode;

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


