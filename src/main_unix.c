
#include "./headers.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        printf("Usage: main_unix [volume path]\n");
        return 1;
    }
    unixVolumePath = (int8_t *)argv[1];
    int8_t tempResult = initializeStorageSpace();
    if (!tempResult) {
        return 1;
    }
    runAppSystem();
    return 0;
}


