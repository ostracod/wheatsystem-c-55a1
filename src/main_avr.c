
#include "./headers.h"

int main(void) {
    initializePinModes();
    initializeSpi();
    initializeSram();
    initializeStorageSpace();
    initializeLcd();
    runAppSystem();
    return 0;
}


