
#include "./headers.h"

const int8_t bootStringConstant[] PROGMEM = "wsBoot";

const int8_t argAmountOffsetArray[] PROGMEM = {
    0, 10, 17, 22, 26, 32, 38, 45, 51, 57, 63, 66, 71, 73
};

const int8_t argAmountArray[] PROGMEM = {
    // Memory instructions.
    2, 3, 3, 1, 3, 1, 2, 2, 2, 2,
    // Control flow instructions.
    1, 2, 2, 3, 3, 3, 3,
    // Gate instructions.
    2, 1, 1, 1, 1,
    // Error instructions.
    1, 0, 1, 1,
    // Function instructions.
    3, 1, 2, 0, 1, 3,
    // Bitwise instructions.
    2, 3, 3, 3, 3, 3,
    // Arithmetic instructions.
    1, 1, 3, 3, 3, 3, 3,
    // Application instructions.
    1, 1, 0, 2, 2, 1,
    // File instructions.
    4, 1, 2, 1, 4, 4,
    // File metadata instructions.
    1, 2, 2, 2, 2, 2,
    // Permission instructions.
    2, 1, 1,
    // Resource instructions.
    1, 2, 1, 1, 1,
    // Test instructions.
    1, 0
};

const systemAppFunc_t termAppFuncArray[] PROGMEM = {
    (systemAppFunc_t){INIT_FUNC_ID, true, 0, 0, initializeTermApp},
    (systemAppFunc_t){LISTEN_TERM_FUNC_ID, true, 0, 0, setTermObserver},
    (systemAppFunc_t){TERM_SIZE_FUNC_ID, true, 8, 0, getTermSize},
    (systemAppFunc_t){WRT_TERM_FUNC_ID, true, 12, 0, writeTermText},
    (systemAppFunc_t){KILL_FUNC_ID, true, 0, 0, killTermApp}
};


