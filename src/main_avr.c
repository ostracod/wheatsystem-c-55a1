
#include "./headers.h"

int main(void) {
    initializePinModes();
    initializeSpi();
    initializeUart();
    initializeSram();
    initializeStorage();
    initializeLcd();
    int8_t button = getPressedButton();
    initializeButtonTimer();
    resetSystemState();
    if (button == ACTIONS_BUTTON) {
        runTransferMode();
    } else {
        runAppSystem();
    }
    return 0;
}


