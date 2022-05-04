
#include <string.h>
#include <stdio.h>

// Retrieves a value within the given fixed array.
// "name" is a fixed array.
// "index" is the offset of byte from which to start reading.
#define readFixedArrayValue(name, index, type) *(type *)((int8_t *)name + index)

// Reads an interval of data from non-volatile storage.
// "address" is the offset of first byte to read.
#define readStorageSpaceRange(destination, address, amount) \
    memcpy(destination, storageSpace + address, amount);

// Retrieves the number of bytes in the given file.
int32_t getNativeFileSize(FILE *fileHandle);

// Writes an interval of data to non-volatile storage. Changes might not be persisted until calling flushStorageSpace.
// "address" is the offset of first byte to write.
void writeStorageSpaceRange(int32_t address, void *source, int32_t amount);
// Persists any pending changes to non-volatile storage.
void flushStorageSpace();


