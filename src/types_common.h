
// Includes all standard integer types.
#include <stdint.h>

// Stores an offset within heap memory. Must accommodate values between negative HEAP_MEMORY_SIZE and positive HEAP_MEMORY_SIZE.
#define heapMemoryOffset_t int16_t
// Stores an offset within volume storage.
#define storageOffset_t int32_t

// Stores a pointer to a heap allocation. allocPointer_t must accommodate the maximum possible pointer value.
#define allocPointer_t int16_t

// Schedules work for a particular function of a system application.
typedef void (*systemAppThreadAction_t)();


