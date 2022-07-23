
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

int8_t initializeStorage() {
    if (unixVolumePath == NULL) {
        clearStorage();
        return true;
    }
    FILE *fileHandle = fopen((char *)unixVolumePath, "r");
    if (fileHandle == NULL) {
        printf("Could not find volume file.\n");
        return false;
    }
    volumeFileSize = getNativeFileSize(fileHandle);
    fread(storageSpace, 1, volumeFileSize, fileHandle);
    fclose(fileHandle);
    storageIsDirty = false;
    return true;
}

void writeStorageRange(storageOffset_t address, void *source, storageOffset_t amount) {
    storageIsDirty = true;
    storageOffset_t endAddress = address + amount;
    if (endAddress > volumeFileSize) {
        volumeFileSize = endAddress;
    }
    memcpy(storageSpace + address, source, amount);
}

void flushStorage() {
    if (!storageIsDirty || unixVolumePath == NULL) {
        return;
    }
    FILE *fileHandle = fopen((char *)unixVolumePath, "w");
    fwrite(storageSpace, 1, volumeFileSize, fileHandle);
    fclose(fileHandle);
    storageIsDirty = false;
}

void initializeTermApp() {
    if (window == NULL) {
        window = initscr();
        noecho();
        curs_set(0);
        keypad(window, true);
        ESCDELAY = 50;
        timeout(0);
        int32_t width;
        int32_t height;
        getmaxyx(window, height, width);
        writeTermAppGlobalVar(width, width);
        writeTermAppGlobalVar(height, height);
    }
    allocPointer_t observer = readTermAppGlobalVar(observer);
    if (observer == NULL_ALLOC_POINTER) {
        return;
    }
    allocPointer_t runningApp = getFileHandleRunningApp(observer);
    if (runningApp == NULL_ALLOC_POINTER) {
        return;
    }
    
    int32_t key = getch();
    if (key < 32 || key > 127) {
        if (key == KEY_LEFT) {
            key = -1;
        } else if (key == KEY_RIGHT) {
            key = -2;
        } else if (key == KEY_UP) {
            key = -3;
        } else if (key == KEY_DOWN) {
            key = -4;
        } else if (key == KEY_BTAB) {
            key = -5;
        } else if (key == KEY_HOME) {
            key = -6;
        } else if (key != 9 && key != 10 && key != 27) {
            return;
        }
    }
    allocPointer_t nextArgFrame = createNextArgFrame(1);
    checkErrorInSystemApp();
    writeArgFrame(nextArgFrame, 0, int8_t, (int8_t)key);
    int32_t termInputIndex = readTermAppGlobalVar(termInputIndex);
    callFunc(currentThread, runningApp, termInputIndex, true);
    checkErrorInSystemApp();
}

void getTermSize() {
    int32_t width = readTermAppGlobalVar(width);
    int32_t height = readTermAppGlobalVar(height);
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    writeArgFrame(previousArgFrame, 0, int32_t, width);
    writeArgFrame(previousArgFrame, 4, int32_t, height);
    returnFromFunc();
}

void writeTermText() {
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    int32_t posX = readArgFrame(previousArgFrame, 0, int32_t);
    int32_t posY = readArgFrame(previousArgFrame, 4, int32_t);
    allocPointer_t textAlloc = readArgFrame(previousArgFrame, 8, int32_t);
    validateDynamicAlloc(textAlloc);
    checkErrorInSystemApp();
    if (!runningAppMayAccessAlloc(getCurrentCaller(), textAlloc)
            || !currentImplementerMayAccessAlloc(textAlloc)) {
        throwInSystemApp(PERM_ERR_CODE);
    }
    heapMemOffset_t textSize = getDynamicAllocSize(textAlloc);
    wmove(window, posY, posX);
    for (heapMemOffset_t index = 0; index < textSize; index++) {
        int8_t character = readDynamicAlloc(textAlloc, index, int8_t);
        waddch(window, (char)character);
    }
    refresh();
    returnFromFunc();
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

void sendTestPacket(testPacket_t packet) {
    write(testSocketHandle, &(packet.type), sizeof(packet.type));
    write(testSocketHandle, &(packet.dataLength), sizeof(packet.dataLength));
    if (packet.data != NULL && packet.dataLength > 0) {
        write(testSocketHandle, packet.data, packet.dataLength);
    }
}

testPacket_t receiveTestPacket() {
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

void clearHeapMem() {
    for (heapMemOffset_t address = 0; address < HEAP_MEM_SIZE; address++) {
        writeHeapMem(address, int8_t, 0);
    }
}

void clearStorage() {
    for (storageOffset_t address = 0; address < STORAGE_SIZE; address++) {
        writeStorage(address, int8_t, 0);
    }
    setStorageMember(firstFileAddress, MISSING_FILE_ADDRESS);
    flushStorage();
}

void createFileByTestPacket(testPacket_t packet) {
    createFilePacketHeader_t *packetHeader = (createFilePacketHeader_t *)packet.data;
    int8_t nameSize = packetHeader->nameSize;
    int32_t nameStartIndex = sizeof(createFilePacketHeader_t);
    int32_t contentStartIndex = nameStartIndex + nameSize;
    storageOffset_t contentSize = packet.dataLength - contentStartIndex;
    allocPointer_t nameAlloc = createStringAllocFromFixedArrayHelper(
        packet.data + nameStartIndex,
        nameSize
    );
    checkUnhandledError();
    createFile(nameAlloc, packetHeader->type, packetHeader->isGuarded, contentSize);
    checkUnhandledError();
    allocPointer_t fileHandle = openFileByStringAlloc(nameAlloc);
    checkUnhandledError();
    deleteAlloc(nameAlloc);
    setFileHasAdminPerm(fileHandle, packetHeader->hasAdminPerm);
    storageOffset_t contentAddress = getFileHandleDataAddress(fileHandle);
    for (storageOffset_t offset = 0; offset < contentSize; offset++) {
        int8_t value = packet.data[contentStartIndex + offset];
        writeStorage(contentAddress + offset, int8_t, value);
    }
    closeFile(fileHandle);
}

void handleTestInstructionHelper(int8_t opcodeOffset) {
    if (opcodeOffset == 0x0) {
        // logTestData.
        int32_t value = readArgInt(0);
        testPacket_t packet = {LOGGED_TEST_PACKET_TYPE, sizeof(value), (int8_t *)&value};
        sendTestPacket(packet);
    } else if (opcodeOffset == 0x1) {
        // haltTest.
        systemShouldHalt = true;
    } else {
        throw(NO_IMPL_ERR_CODE);
    }
}

int8_t runIntegrationTests(int8_t *socketPath) {
    isIntegrationTest = true;
    printf("Running integration test mode...\n");
    int8_t result = connectToTestSocket(socketPath);
    if (!result) {
        printf("Unable to connect to test socket.\n");
        return false;
    }
    testPacket_t launchedPacket = {LAUNCHED_TEST_PACKET_TYPE, 0, NULL};
    sendTestPacket(launchedPacket);
    int8_t hasFinished = false;
    int8_t hasError = false;
    while (!hasFinished && !hasError) {
        testPacket_t inputPacket = receiveTestPacket();
        switch (inputPacket.type) {
            case RESET_TEST_PACKET_TYPE: {
                clearHeapMem();
                clearStorage();
                resetSystemState();
                break;
            }
            case CREATE_FILE_TEST_PACKET_TYPE: {
                createFileByTestPacket(inputPacket);
                checkUnhandledError(false);
                break;
            }
            case START_TEST_PACKET_TYPE: {
                runAppSystem();
                testPacket_t haltedPacket = {HALTED_TEST_PACKET_TYPE, 0, NULL};
                sendTestPacket(haltedPacket);
                break;
            }
            case CREATE_ALLOC_TEST_PACKET_TYPE: {
                heapMemOffset_t size = *(int32_t *)inputPacket.data;
                createdAllocPacketBody_t body;
                body.pointer = createAlloc(TEST_ALLOC_TYPE, size);
                checkUnhandledError(false);
                heapMemOffset_t spanAddress = getAllocSpanAddress(body.pointer);
                heapMemOffset_t spanSize = getSpanMember(spanAddress, size);
                body.startAddress = spanAddress;
                body.endAddress = spanAddress + sizeof(spanHeader_t) + spanSize;
                testPacket_t createdPacket = {
                    CREATED_ALLOC_TEST_PACKET_TYPE,
                    sizeof(createdAllocPacketBody_t),
                    (int8_t *)&body
                };
                sendTestPacket(createdPacket);
                break;
            }
            case DELETE_ALLOC_TEST_PACKET_TYPE: {
                allocPointer_t pointer = *(int32_t *)inputPacket.data;
                deleteAlloc(pointer);
                testPacket_t deletedPacket = {DELETED_ALLOC_TEST_PACKET_TYPE, 0, NULL};
                sendTestPacket(deletedPacket);
                break;
            }
            case VALIDATE_ALLOC_TEST_PACKET_TYPE: {
                allocPointer_t pointer = *(int32_t *)inputPacket.data;
                validateAllocPointer(pointer);
                testPacket_t validatedPacket = {
                    VALIDATED_ALLOC_TEST_PACKET_TYPE,
                    1,
                    &unhandledErrorCode
                };
                sendTestPacket(validatedPacket);
                unhandledErrorCode = NONE_ERR_CODE;
                break;
            }
            case QUIT_TEST_PACKET_TYPE: {
                hasFinished = true;
                break;
            }
            default: {
                printf("Unrecognized test packet type %d!\n", inputPacket.type);
                hasError = true;
            }
        }
        if (inputPacket.data != NULL) {
            free(inputPacket.data);
        }
    }
    return !hasError;
}


