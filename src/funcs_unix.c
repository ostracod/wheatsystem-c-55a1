
#include "./headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    storageSpaceSize = getNativeFileSize(fileHandle);
    storageSpace = malloc(storageSpaceSize);
    fread(storageSpace, 1, storageSpaceSize, fileHandle);
    fclose(fileHandle);
    return true;
}

void writeStorageSpaceRange(int32_t address, void *source, int32_t amount) {
    storageSpaceIsDirty = true;
    int32_t endAddress = address + amount;
    if (endAddress > storageSpaceSize) {
        storageSpaceSize = endAddress + 10 * 1000;
        storageSpace = realloc(storageSpace, storageSpaceSize);
    }
    memcpy(storageSpace + address, source, amount);
}

void flushStorageSpace() {
    if (!storageSpaceIsDirty) {
        return;
    }
    FILE *fileHandle = fopen((char *)unixVolumePath, "w");
    fwrite(storageSpace, 1, storageSpaceSize, fileHandle);
    fclose(fileHandle);
    storageSpaceIsDirty = false;
}

void initializeTermApp() {
    // TODO: Implement.
}

void setTermObserver() {
    // TODO: Implement.
}

void getTermSize() {
    // TODO: Implement.
}

void writeTermText() {
    // TODO: Implement.
}


