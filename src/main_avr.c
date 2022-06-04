
#include "./headers.h"

int main(void) {
    initializePinModes();
    initializeSpi();
    initializeUart();
    initializeSram();
    initializeStorageSpace();
    initializeLcd();
    runAppSystem();
    return 0;
}


