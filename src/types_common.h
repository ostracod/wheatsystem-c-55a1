
// Includes all standard integer types.
#include <stdint.h>

// Stores an offset within heap memory. Must accommodate values between negative HEAP_MEM_SIZE and positive HEAP_MEM_SIZE.
#define heapMemOffset_t int16_t
// Stores an offset within volume storage.
#define storageOffset_t int32_t

// Stores the start address of the allocHeader_t belonging to a heap allocation. allocPointer_t must accommodate the maximum possible pointer value.
#define allocPointer_t int16_t

// allocIterationHandle_t accepts a context pointer, a pointer to an allocation, and a pointer to the runningApp_t owner of the allocation.
typedef void (*allocIterationHandle_t)(void *, allocPointer_t, allocPointer_t);

// Schedules work for a particular function of a system application.
typedef void (*systemAppThreadAction_t)();


