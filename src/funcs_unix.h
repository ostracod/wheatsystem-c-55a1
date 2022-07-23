
#include <string.h>
#include <stdio.h>

// Retrieves a value within the given fixed array.
// "name" is a fixed array.
// "index" is the offset of byte from which to start reading.
#define readFixedArrayValue(name, index, type) *(type *)((int8_t *)name + index)

// Reads an interval of data from heap memory.
// "address" is the offset of first byte to read.
#define readHeapMemRange(destination, address, amount) memcpy(destination, heapMem + address, amount)
// Writes an interval of data to heap memory.
// "address" is the offset of first byte to write.
#define writeHeapMemRange(address, source, amount) memcpy(heapMem + address, source, amount)

// Reads a value from heap memory.
// "address" is the address of first byte to read.
#define readHeapMem(address, type) (*(type *)(heapMem + address))
// Writes a value to heap memory.
// "address" is the address of first byte to write.
#define writeHeapMem(address, type, value) (*(type *)(heapMem + address) = value)

// Reads an interval of data from non-volatile storage.
// "address" is the offset of first byte to read.
#define readStorageRange(destination, address, amount) \
    memcpy(destination, storageSpace + address, amount);

#define resetHaltFlag() (systemShouldHalt = false)
#define checkHaltFlag() if (systemShouldHalt) { \
    break; \
}

#define handleTestInstruction() \
    if (opcodeCategory == 0xC) { \
        handleTestInstructionHelper(opcodeOffset); \
        return; \
    }

// Retrieves the number of bytes in the given file.
int32_t getNativeFileSize(FILE *fileHandle);

// Initializes non-volatile storage. Must be called before using non-volatile storage.
// Returns whether non-volatile storage was initialized successfully.
int8_t initializeStorage();
// Writes an interval of data to non-volatile storage. Changes might not be persisted until calling flushStorage.
// "address" is the offset of first byte to write.
void writeStorageRange(storageOffset_t address, void *source, storageOffset_t amount);
// Persists any pending changes to non-volatile storage.
void flushStorage();

void sleepMilliseconds(int32_t milliseconds);

void printUnixUsage();
int8_t connectToTestSocket(int8_t *path);
void sendTestPacket(testPacket_t packet);
testPacket_t receiveTestPacket();
void clearHeapMem();
void clearStorage();
void createFileByTestPacket(testPacket_t packet);
void handleTestInstructionHelper(int8_t opcodeOffset);
int8_t runIntegrationTests(int8_t *socketPath);


