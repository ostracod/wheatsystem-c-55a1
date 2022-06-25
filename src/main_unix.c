
#include "./headers.h"
#include <stdio.h>
#include <string.h>

int main(int argc, const char *argv[]) {
    if (argc == 2) {
        resetSystemState();
        unixVolumePath = (int8_t *)argv[1];
        int8_t result = initializeStorageSpace();
        if (!result) {
            return 1;
        }
        runAppSystem();
    } else if (argc == 3) {
        if (strcmp(argv[1], "--integration-test") != 0) {
            printUnixUsage();
            return 1;
        }
        unixVolumePath = NULL;
        int8_t result = runIntegrationTests((int8_t *)argv[2]);
        if (!result) {
            return 1;
        }
    } else {
        printUnixUsage();
        return 1;
    }
    return 0;
}


