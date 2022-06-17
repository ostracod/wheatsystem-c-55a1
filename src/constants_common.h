
#define true 1
#define false 0

// Size of memory available in the heap.
#define HEAP_MEMORY_SIZE 32767

// Has the type allocPointer_t.
#define NULL_ALLOC_POINTER 0

#define RUNNING_APP_ALLOC_TYPE 0
#define THREAD_ALLOC_TYPE 1
#define LOCAL_FRAME_ALLOC_TYPE 2
#define ARG_FRAME_ALLOC_TYPE 3
#define DYNAMIC_ALLOC_TYPE 4

#define GUARDED_ALLOC_ATTR 0x01
#define SENTRY_ALLOC_ATTR 0x02

// Size available to store files.
#define STORAGE_SPACE_SIZE ((int32_t)128 * (int32_t)1024)

#define MISSING_FILE_ADDRESS 0

#define NONE_FILE_TYPE -1
#define GENERIC_FILE_TYPE 0
#define BYTECODE_APP_FILE_TYPE 1
#define SYSTEM_APP_FILE_TYPE 2

#define GUARDED_FILE_ATTR 0x04
#define ADMIN_FILE_ATTR 0x08

#define NONE_ERR_CODE 0x00
#define GENERIC_ERR_CODE 0x01
#define NO_IMPL_ERR_CODE 0x02
#define TYPE_ERR_CODE 0x03
#define NUM_RANGE_ERR_CODE 0x04
#define INDEX_ERR_CODE 0x05
#define PTR_ERR_CODE 0x06
#define NULL_ERR_CODE 0x07
#define DATA_ERR_CODE 0x08
#define ARG_FRAME_ERR_CODE 0x09
#define MISSING_ERR_CODE 0x0A
#define STATE_ERR_CODE 0x0B
#define PERM_ERR_CODE 0x0C
#define CAPACITY_ERR_CODE 0x0D
#define THROTTLE_ERR_CODE 0x0E

#define KILL_STATES_PERIOD 50

#define NONE_KILL_ACTION 0
#define WARN_KILL_ACTION 1
#define THROTTLE_KILL_ACTION 2
#define HARD_KILL_ACTION 3

#define CONSTANT_REF_TYPE 0x0
#define GLOBAL_FRAME_REF_TYPE 0x1
#define LOCAL_FRAME_REF_TYPE 0x2
#define PREV_ARG_FRAME_REF_TYPE 0x3
#define NEXT_ARG_FRAME_REF_TYPE 0x4
#define APP_DATA_REF_TYPE 0x5
#define DYNAMIC_ALLOC_REF_TYPE 0x6
#define HEAP_MEM_REF_TYPE 0x7

#define SIGNED_INT_8_TYPE 0x0
#define SIGNED_INT_32_TYPE 0x1

#define INIT_FUNC_ID 1
#define KILL_FUNC_ID 2
#define LISTEN_TERM_FUNC_ID 3
#define TERM_SIZE_FUNC_ID 4
#define WRT_TERM_FUNC_ID 5
#define TERM_INPUT_FUNC_ID 6
#define START_SERIAL_FUNC_ID 7
#define STOP_SERIAL_FUNC_ID 8
#define WRT_SERIAL_FUNC_ID 9
#define SERIAL_INPUT_FUNC_ID 10
#define SET_GPIO_MODE_FUNC_ID 11
#define READ_GPIO_FUNC_ID 12
#define WRT_GPIO_FUNC_ID 13

// The maximum number of arguments which any bytecode instruction accepts.
#define MAXIMUM_ARG_AMOUNT 4

// Global frame size of a terminal driver.
#define TERM_APP_GLOBAL_FRAME_SIZE sizeof(termAppGlobalFrame_t)

// Fixed array of characters.
const int8_t bootStringConstant[5];

// Mapping from more significant opcode nybble to offset in argumentAmountArray. Fixed array of values.
const int8_t argumentAmountOffsetArray[12];
// Expected number of arguments for each bytecode instruction. Fixed array of values.
const int8_t argumentAmountArray[63];

// List of functions which the term driver implements. Fixed array of values.
const systemAppFunction_t termAppFunctionArray[4];

// Determines the system apps which are available on the current platform. Fixed array of values.
const systemApp_t systemAppArray[1];


