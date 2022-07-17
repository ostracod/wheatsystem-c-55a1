
#include "./headers.h"

int main(void) {
    initializePinModes();
    initializeSpi();
    initializeUart();
    initializeSram();
    initializeStorage();
    initializeLcd();
    resetSystemState();
    int8_t button = getPressedButton();
    if (button == ACTION_BUTTON) {
        runTransferMode();
    } else {
        runAppSystem();
    }
    return 0;
}


