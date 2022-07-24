
#include "./headers.h"

const int8_t lcdInitCommands[] PROGMEM = {
    0x39, 0x14, 0x55, 0x6D, 0x78, 0x38, 0x0C, 0x01, 0x06
};

const int8_t transferModeStringConstant[] PROGMEM = "Transfer mode\nis active.";

const int8_t charsModeKeyCodes[] PROGMEM = {
    'A', 'D', 'G', 'J', 'M', 'P', 'S', 'U', 'W', 'Y',
    'B', 'E', 'H', 'K', 'N', 'Q', 'T', 'V', 'X', 'Z',
    'C', 'F', 'I', 'L', 'O', 'R', '<', '[', '(', '{',
    'a', 'd', 'g', 'j', 'm', 'p', 's', 'u', 'w', 'y',
    'b', 'e', 'h', 'k', 'n', 'q', 't', 'v', 'x', 'z',
    'c', 'f', 'i', 'l', 'o', 'r', '>', ']', ')', '}',
    9,   '#', -5,  '&', 10,  39,  ' ', '+', 127, 27,
    '~', '$', ':', '|', '/', '"', '=', '-', '.', '?',
    92,  '@', ';', '^', '%', '`', '_', '*', ',', '!',
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
};

const int8_t actionsModeKeyCodes[] PROGMEM = {
    9,   -3,  -5,  -1,  10,  -2,  -6,  -4,  127, 27
};

const systemAppFunc_t serialAppFuncArray[] PROGMEM = {
    (systemAppFunc_t){INIT_FUNC_ID, true, 0, 0, initializeSerialApp},
    (systemAppFunc_t){START_SERIAL_FUNC_ID, true, 5, 0, startSerialApp},
    (systemAppFunc_t){STOP_SERIAL_FUNC_ID, true, 1, 0, stopSerialApp},
    (systemAppFunc_t){WRT_SERIAL_FUNC_ID, true, 5, 0, writeSerialApp},
    (systemAppFunc_t){KILL_FUNC_ID, true, 0, 0, killSerialApp}
};

const systemApp_t systemAppArray[] PROGMEM = {
    createSystemApp(TERM_APP_GLOBAL_FRAME_SIZE, termAppFuncArray),
    createSystemApp(SERIAL_APP_GLOBAL_FRAME_SIZE, serialAppFuncArray)
};


