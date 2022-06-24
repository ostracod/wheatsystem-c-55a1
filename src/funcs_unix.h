
#include <string.h>
#include <stdio.h>

// Retrieves a value within the given fixed array.
// "name" is a fixed array.
// "index" is the offset of byte from which to start reading.
#define readFixedArrayValue(name, index, type) *(type *)((int8_t *)name + index)

// Reads a value from heap memory.
// "address" is the address of first byte to read.
#define readHeapMemory(address, type) (*(type *)(heapMemory + address))
// Writes a value to heap memory.
// "address" is the address of first byte to write.
#define writeHeapMemory(address, type, value) (*(type *)(heapMemory + address) = value)

// Reads an interval of data from non-volatile storage.
// "address" is the offset of first byte to read.
#define readStorageSpaceRange(destination, address, amount) \
    memcpy(destination, storageSpace + address, amount);

// Retrieves the number of bytes in the given file.
int32_t getNativeFileSize(FILE *fileHandle);

// Initializes non-volatile storage. Must be called before using non-volatile storage. Returns false if non-volatile storage could not be initialized successfully.
int8_t initializeStorageSpace();
// Writes an interval of data to non-volatile storage. Changes might not be persisted until calling flushStorageSpace.
// "address" is the offset of first byte to write.
void writeStorageSpaceRange(storageOffset_t address, void *source, storageOffset_t amount);
// Persists any pending changes to non-volatile storage.
void flushStorageSpace();

void handleWindowResize();

void sleepMilliseconds(int32_t milliseconds);

void printUnixUsage();
int8_t connectToTestSocket(int8_t *path);
void writeToTestSocket(int8_t *data, int32_t length);
int8_t *readFromTestSocket(int32_t *lengthDestination);
int8_t runIntegrationTests(int8_t *socketPath);


