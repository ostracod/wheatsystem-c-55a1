
#include "./headers.h"

const int8_t bootStringConstant[] PROGMEM = "boot";

const int8_t argumentAmountOffsetArray[] PROGMEM = {
    0, 8, 13, 17, 22, 28, 32, 37, 43, 49, 55, 58
};

const int8_t argumentAmountArray[] PROGMEM = {
    // Memory instructions.
    2, 3, 1, 3, 1, 2, 2, 2,
    // Control flow instructions.
    1, 2, 2, 0, 0,
    // Error instructions.
    1, 0, 1, 1,
    // Function instructions.
    3, 1, 2, 0, 1,
    // Bitwise instructions.
    2, 3, 3, 3, 3, 3,
    // Comparison instructions.
    3, 3, 3, 3,
    // Arithmetic instructions.
    3, 3, 3, 3, 3,
    // Application instructions.
    1, 1, 0, 2, 2, 1,
    // File instructions.
    4, 1, 2, 1, 4, 4,
    // File metadata instructions.
    1, 2, 2, 2, 2, 2,
    // Permission instructions.
    2, 1, 1,
    // Resource instructions.
    1, 2, 1, 1, 1
};

const systemAppFunction_t termAppFunctionArray[] PROGMEM = {
    (systemAppFunction_t){INIT_FUNC_ID, true, 0, 0, initializeTermApp},
    (systemAppFunction_t){LISTEN_TERM_FUNC_ID, true, 0, 0, setTermObserver},
    (systemAppFunction_t){TERM_SIZE_FUNC_ID, true, 8, 0, getTermSize},
    (systemAppFunction_t){WRT_TERM_FUNC_ID, true, 12, 0, writeTermText}
};

const systemApp_t systemAppArray[] PROGMEM = {
    createSystemApp(TERM_APP_GLOBAL_FRAME_SIZE, termAppFunctionArray)
};


