
#include "./headers.h"

heapMemOffset_t emptySpansByDegree[SPAN_DEGREE_AMOUNT];
uint8_t emptySpanBitField[SPAN_BIT_FIELD_SIZE];
allocPointer_t allocsByType[ALLOC_TYPE_AMOUNT];
uint8_t allocBitField[ALLOC_BIT_FIELD_SIZE];
heapMemOffset_t heapMemSizeLeft;

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


