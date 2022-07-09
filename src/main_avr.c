
#include "./headers.h"

int main(void) {
    resetSystemState();
    initializePinModes();
    initializeSpi();
    initializeUart();
    initializeSram();
    initializeStorage();
    initializeLcd();
    int8_t button = getPressedButton();
    if (button == ACTION_BUTTON) {
        runTransferMode();
    } else {
        runAppSystem();
    }
    return 0;
}


