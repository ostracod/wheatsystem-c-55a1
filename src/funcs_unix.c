
#include "./headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int32_t getNativeFileSize(FILE *fileHandle) {
    fseek(fileHandle, 0, SEEK_END);
    int32_t output = (int32_t)ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_SET);
    return output;
}

int8_t initializeStorageSpace() {
    FILE *fileHandle = fopen((char *)unixVolumePath, "r");
    if (fileHandle == NULL) {
        printf("Could not find volume file.\n");
        return false;
    }
    volumeFileSize = getNativeFileSize(fileHandle);
    storageSpace = malloc(STORAGE_SPACE_SIZE);
    fread(storageSpace, 1, volumeFileSize, fileHandle);
    fclose(fileHandle);
    return true;
}

void writeStorageSpaceRange(storageOffset_t address, void *source, storageOffset_t amount) {
    storageSpaceIsDirty = true;
    storageOffset_t endAddress = address + amount;
    if (endAddress > volumeFileSize) {
        volumeFileSize = endAddress;
    }
    memcpy(storageSpace + address, source, amount);
}

void flushStorageSpace() {
    if (!storageSpaceIsDirty) {
        return;
    }
    FILE *fileHandle = fopen((char *)unixVolumePath, "w");
    fwrite(storageSpace, 1, volumeFileSize, fileHandle);
    fclose(fileHandle);
    storageSpaceIsDirty = false;
}

void handleWindowResize() {
    int32_t tempWidth;
    int32_t tempHeight;
    getmaxyx(window, tempHeight, tempWidth);
    writeTermAppGlobalVariable(width, tempWidth);
    writeTermAppGlobalVariable(height, tempHeight);
}

void initializeTermApp() {
    if (window == NULL) {
        window = initscr();
        noecho();
        curs_set(0);
        keypad(window, true);
        ESCDELAY = 50;
        timeout(0);
        handleWindowResize();
    }
    allocPointer_t tempObserver = readTermAppGlobalVariable(observer);
    if (tempObserver == NULL_ALLOC_POINTER) {
        return;
    }
    // TODO: Check whether observer is still running.
    
    int32_t tempKey = getch();
    if (tempKey < 32 || tempKey > 127) {
        if (tempKey == KEY_LEFT) {
            tempKey = -1;
        } else if (tempKey == KEY_RIGHT) {
            tempKey = -2;
        } else if (tempKey == KEY_UP) {
            tempKey = -3;
        } else if (tempKey == KEY_DOWN) {
            tempKey = -4;
        } else if (tempKey == 263) {
            tempKey = 127;
        } else if (tempKey != 10 && tempKey != 27) {
            return;
        }
    }
    allocPointer_t nextArgFrame = createNextArgFrame(1);
    checkUnhandledError();
    writeArgFrame(nextArgFrame, 0, int8_t, (int8_t)tempKey);
    int32_t termInputIndex = readTermAppGlobalVariable(termInputIndex);
    callFunction(currentThread, tempObserver, termInputIndex, true);
}

void setTermObserver() {
    allocPointer_t tempCaller = getCurrentCaller();
    int32_t termInputIndex = findFunctionById(tempCaller, TERM_INPUT_FUNC_ID);
    if (termInputIndex < 0) {
        unhandledErrorCode = MISSING_ERR_CODE;
    } else {
        writeTermAppGlobalVariable(observer, tempCaller);
        writeTermAppGlobalVariable(termInputIndex, termInputIndex);
    }
    returnFromFunction();
}

void getTermSize() {
    int32_t tempWidth = readTermAppGlobalVariable(width);
    int32_t tempHeight = readTermAppGlobalVariable(height);
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    writeArgFrame(previousArgFrame, 0, int32_t, tempWidth);
    writeArgFrame(previousArgFrame, 4, int32_t, tempHeight);
    returnFromFunction();
}

void writeTermText() {
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    int32_t posX = readArgFrame(previousArgFrame, 0, int32_t);
    int32_t posY = readArgFrame(previousArgFrame, 4, int32_t);
    allocPointer_t textAlloc = readArgFrame(previousArgFrame, 8, int32_t);
    if (!currentImplementerMayAccessAlloc(textAlloc)) {
        unhandledErrorCode = PERM_ERR_CODE;
        returnFromFunction();
        return;
    }
    heapMemoryOffset_t textSize = getDynamicAllocSize(textAlloc);
    wmove(window, posY, posX);
    for (heapMemoryOffset_t index = 0; index < textSize; index++) {
        int8_t tempCharacter = readDynamicAlloc(textAlloc, index, int8_t);
        waddch(window, (char)tempCharacter);
    }
    refresh();
    returnFromFunction();
}

void sleepMilliseconds(int32_t milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void printUnixUsage() {
    printf("Usage:\n./main_unix [volume path]\n./main_unix --integration-test [socket path]\n");
}

int8_t connectToTestSocket(int8_t *path) {
    testSocketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sun_family = AF_UNIX;
    strncpy(sockAddr.sun_path, (char *)path, sizeof(sockAddr.sun_path) - 1);
    int32_t result = connect(
        testSocketHandle,
        (struct sockaddr *)&sockAddr,
        sizeof(sockAddr)
    );
    return (result == 0);
}

void writeToTestSocket(testPacket_t packet) {
    write(testSocketHandle, &(packet.type), sizeof(packet.type));
    write(testSocketHandle, &(packet.dataLength), sizeof(packet.dataLength));
    if (packet.data != NULL && packet.dataLength > 0) {
        write(testSocketHandle, packet.data, packet.dataLength);
    }
}

testPacket_t readFromTestSocket() {
    testPacket_t output;
    read(testSocketHandle, &(output.type), sizeof(output.type));
    read(testSocketHandle, &(output.dataLength), sizeof(output.dataLength));
    if (output.dataLength > 0) {
        output.data = malloc(output.dataLength);
        read(testSocketHandle, output.data, output.dataLength);
    } else {
        output.data = NULL;
    }
    return output;
}

int8_t runIntegrationTests(int8_t *socketPath) {
    isIntegrationTest = true;
    int8_t result = connectToTestSocket(socketPath);
    if (!result) {
        printf("Unable to connect to test socket.\n");
        return false;
    }
    int32_t testValue = 0;
    while (true) {
        testPacket_t inputPacket = readFromTestSocket();
        printf(
            "Received socket data!\nType: %d; Length: %d\n",
            inputPacket.type, inputPacket.dataLength
        );
        if (inputPacket.data != NULL) {
            free(inputPacket.data);
        }
        printf("Sending socket data...\n");
        testPacket_t outputPacket = {LOGGED_TEST_PACKET_TYPE, 4, (int8_t *)&testValue};
        writeToTestSocket(outputPacket);
        testValue += 1;
    }
    return true;
}


