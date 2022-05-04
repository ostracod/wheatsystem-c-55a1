
// Retrieves the number of elements in the given array.
#define getArrayLength(name) (int32_t)(sizeof(name) / sizeof(*name))
#define getArrayElementOffset(name, index) (index * sizeof(name))
#define getArrayElementType(name) typeof(*name)

// Retrieves the type of "memberName" in "structDefinition".
#define getStructMemberType(structDefinition, memberName) \
    typeof(((structDefinition *)NULL)->memberName)
// Retrieves the location of "memberName" in "structDefinition", measured as the number of bytes from the beginning of the struct.
#define getStructMemberOffset(structDefinition, memberName) \
    (int32_t)((void *)&((structDefinition *)NULL)->memberName - NULL)
// Reads an element within the given fixed array.
// "index" is the number of elements from the beginning of the array.
#define readFixedArrayElement(fixedArray, index) \
    readFixedArrayValue(fixedArray, getArrayElementOffset(fixedArray, index), getArrayElementType(fixedArray))

// Reads a value from heap memory.
// "address" is the address of first byte to read.
#define readHeapMemory(address, type) (*(type *)(heapMemory + address))
// Writes a value to heap memory.
// "address" is the address of first byte to write.
#define writeHeapMemory(address, type, value) (*(type *)(heapMemory + address) = value)

// We need the zero pointer to be null, so we offset all addresses by one.
#define convertPointerToAddress(pointer) (pointer - 1)
#define convertAddressToPointer(address) (address + 1)

// Retrieves the address of an allocation header member in the given allocation.
// "pointer" is an allocPointer_t.
// "memberName" is the name of a member in allocHeader_t.
#define getAllocMemberAddress(pointer, memberName) \
    convertPointerToAddress(pointer) + getStructMemberOffset(allocHeader_t, memberName)
// Retrieves a member of the allocation header in the given allocation.
// "pointer" is an allocPointer_t.
// "memberName" is the name of a member in allocHeader_t.
#define getAllocMember(pointer, memberName) \
    readHeapMemory(getAllocMemberAddress(pointer, memberName), getStructMemberType(allocHeader_t, memberName))
// Modifies a member of the allocation header in the given allocation.
// "pointer" is an allocPointer_t.
// "memberName" is the name of a member in allocHeader_t.
#define setAllocMember(pointer, memberName, value) \
    writeHeapMemory(getAllocMemberAddress(pointer, memberName), getStructMemberType(allocHeader_t, memberName), value)

// Retrieves the start address of the data region in the given allocation.
#define getAllocDataAddress(pointer) \
    (convertPointerToAddress(pointer) + sizeof(allocHeader_t))
// Reads a value from the data region in the given allocation.
// "pointer" is an allocPointer_t.
// "index" is the offset of value in the data region.
#define readAlloc(pointer, index, type) \
    readHeapMemory(getAllocDataAddress(pointer) + index, type)
// Write a value to the data region in the given allocation.
// "pointer" is an allocPointer_t.
// "index" is the offset of value in the data region.
#define writeAlloc(pointer, index, type, value) \
    writeHeapMemory(getAllocDataAddress(pointer) + index, type, value)

// Retrieves the type of the given allocation.
#define getAllocType(pointer) getAllocMember(pointer, type)
// Retrieves the size of the data region in the given allocation.
#define getAllocSize(pointer) getAllocMember(pointer, size)
// Retrieves the allocation after the given allocation in the linked list.
#define getAllocNext(pointer) getAllocMember(pointer, next)

// Retrieves a member of the dynamic allocation header in the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
// "memberName" is the name of a member in dynamicAllocHeader_t.
#define getDynamicAllocMember(pointer, memberName) \
    readAlloc(pointer, getStructMemberOffset(dynamicAllocHeader_t, memberName), getStructMemberType(dynamicAllocHeader_t, memberName))
// Modifies a member of the dynamic allocation header in the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
// "memberName" is the name of a member in dynamicAllocHeader_t.
#define setDynamicAllocMember(pointer, memberName, value) \
    writeAlloc(pointer, getStructMemberOffset(dynamicAllocHeader_t, memberName), getStructMemberType(dynamicAllocHeader_t, memberName), value)

// Retrieves the address of the data region in the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
#define getDynamicAllocDataAddress(pointer) \
    (getAllocDataAddress(pointer) + sizeof(dynamicAllocHeader_t))
// Reads a value from the data region of the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
// "index" is the integer offset of first byte to read.
#define readDynamicAlloc(pointer, index, type) \
    readHeapMemory(getDynamicAllocDataAddress(pointer) + index, type)
// Writes a value to the data region of the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
// "index" is the integer offset of first byte to write.
#define writeDynamicAlloc(pointer, index, type, value) \
    writeHeapMemory(getDynamicAllocDataAddress(pointer) + index, type, value)

// Retrieves the size of the data region in the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
#define getDynamicAllocSize(pointer) \
    (getAllocSize(pointer) - sizeof(dynamicAllocHeader_t))

// Creates a dynamic allocation whose data region contains the string in the given fixed array.
// "fixedArray" is a fixed array of int8_t ending in the null character.
#define createStringAllocFromFixedArray(fixedArray) \
    createStringAllocFromFixedArrayHelper(fixedArray, (heapMemoryOffset_t)(getArrayLength(fixedArray) - 1))

// Reads a value from non-volatile storage.
// "address" is the offset of first byte to read.
#define readStorageSpace(address, type) \
    ({type result; readStorageSpaceRange(&result, address, sizeof(type)); result;})
// Writes a value to non-volatile storage. Changes might not be persisted until calling flushStorageSpace.
// "address" is the offset of first byte to write.
#define writeStorageSpace(address, type, value) \
    ({type tempValue = value; writeStorageSpaceRange(address, &tempValue, sizeof(type));})

// Retrieves the address of the first file in the linked list.
#define getFirstFileAddress() readStorageSpace(0, int32_t)
// Retrieves the name address of the given file.
#define getFileNameAddress(fileAddress) (fileAddress + sizeof(fileHeader_t))

// Retrieves a member of the file header at the given storage address.
// "memberName" is the name of a member in fileHeader_t.
#define getFileHeaderMember(address, memberName) \
    readStorageSpace(address + getStructMemberOffset(fileHeader_t, memberName), getStructMemberType(fileHeader_t, memberName))
// Modifies a member of the file header at the given storage address.
// "memberName" is the name of a member in fileHeader_t.
#define setFileHeaderMember(address, memberName, value) \
    writeStorageSpace(address + getStructMemberOffset(fileHeader_t, memberName), getStructMemberType(fileHeader_t, memberName), value)

// Retrieves a member of the given file handle.
// "fileHandle" is an allocPointer_t to a dynamicAlloc_t.
// "memberName" is the name of a member in fileHandle_t.
#define getFileHandleMember(fileHandle, memberName) \
    readDynamicAlloc(fileHandle, getStructMemberOffset(fileHandle_t, memberName), getStructMemberType(fileHandle_t, memberName))
// Modifies a member of the given file handle.
// "fileHandle" is an allocPointer_t to a dynamicAlloc_t.
// "memberName" is the name of a member in fileHandle_t.
#define setFileHandleMember(fileHandle, memberName, value) \
    writeDynamicAlloc(fileHandle, getStructMemberOffset(fileHandle_t, memberName), getStructMemberType(fileHandle_t, memberName), value)

// Retrieves the type of the given file.
#define getFileHandleType(fileHandle) \
    getTypeFromFileAttributes(getFileHandleMember(fileHandle, attributes))
// Retrieves the size of the data region in the given file.
#define getFileHandleSize(fileHandle) getFileHandleMember(fileHandle, contentSize)
// Retrieves the running app of the given file. Returns NULL_ALLOC_POINTER if the file has no associated running app.
#define getFileHandleRunningApp(fileHandle) getFileHandleMember(fileHandle, runningApp)
// Retrieves the last unhandled error code when launching an app from the given file. Returns 0 if there is no such error.
#define getFileHandleInitErr(fileHandle) getFileHandleMember(fileHandle, initErr)

// Assigns a running app to the given file.
#define setFileHandleRunningApp(fileHandle, runningAppValue) \
    setFileHandleMember(fileHandle, runningApp, runningAppValue)
// Assigns an initialization error code to the given file.
#define setFileHandleInitErr(fileHandle, initErrValue) \
    setFileHandleMember(fileHandle, initErr, initErrValue)

// Reads a value from the data region in the given file.
// "pos" is the offset of first byte to read.
#define readFile(fileHandle, pos, type) \
    ({type result; readFileRange(&result, fileHandle, pos, sizeof(type)); result;})

// Retrieves whether a file has admin permission from the given bit field.
#define getHasAdminPermFromFileAttributes(fileAttributes) ((fileAttributes & 0x08) > 0)
// Retrieves whether a file is guarded from the given bit field.
#define getIsGuardedFromFileAttributes(fileAttributes) ((fileAttributes & 0x04) > 0)
// Retrieves the type of a file from the given bit field.
#define getTypeFromFileAttributes(fileAttributes) (fileAttributes & 0x03)

// Convenience function to create a system application.
// "systemAppFunctionArray" is a fixed array of systemAppFunction_t.
#define createSystemApp(globalFrameSize, systemAppFunctionArray) \
    (systemApp_t){globalFrameSize, systemAppFunctionArray, getArrayLength(systemAppFunctionArray)}

// Creates a heap allocation.
// "size" is the size of data region in the new allocation.
allocPointer_t createAlloc(int8_t type, heapMemoryOffset_t size);
// Frees the given heap allocation.
int8_t deleteAlloc(allocPointer_t pointer);
// Verifies whether the given pointer is valid. May set unhandledErrorCode to NULL_ERR_CODE or PTR_ERR_CODE.
void validateAllocPointer(allocPointer_t pointer);

// Returns an allocPointer_t to a dynamicAlloc_t.
allocPointer_t createDynamicAlloc(
    // Number of bytes in the data region of the new dynamic allocation.
    heapMemoryOffset_t size,
    int8_t attributes,
    // Pointer to fileHandle_t of an app.
    allocPointer_t creator
);
// Returns an allocPointer_t to a dynamicAlloc_t.
allocPointer_t createStringAllocFromFixedArrayHelper(
    // Fixed array of characters.
    const int8_t *fixedArray,
    heapMemoryOffset_t size
);
// Verifies whether the given pointer references a valid dynamic allocation. May assign a new value to unhandledErrorCode.
void validateDynamicAlloc(
    // Pointer to dynamicAlloc_t.
    allocPointer_t dynamicAlloc
);

// Initializes non-volatile storage. Must be called before using non-volatile storage. Returns false if non-volatile storage could not be initialized successfully.
int8_t initializeStorageSpace();

// Determines whether a file name in heap memory equals a file name in storage.
int8_t memoryNameEqualsStorageName(
    heapMemoryOffset_t memoryNameAddress,
    uint8_t memoryNameSize,
    int32_t storageNameAddress,
    uint8_t storageNameSize
);
// Copies a file name from storage to heap memory.
void copyStorageNameToMemory(
    int32_t storageNameAddress,
    heapMemoryOffset_t memoryNameAddress,
    uint8_t nameSize
);

// Opens the file with the given name, returning a file handle. If the file has already been opened, this function returns the existing file handle and increments its open depth. If the file is missing, this function returns NULL_ALLOC_POINTER.
allocPointer_t openFile(heapMemoryOffset_t nameAddress, heapMemoryOffset_t nameSize);
// Closes the given file, decrementing the open depth of the file handle. If the open depth reaches zero, the file handle is deleted.
// "fileHandle" is a pointer to a dynamicAlloc_t.
void closeFile(allocPointer_t fileHandle);
void readFileRange(
    void *destination,
    // Pointer to a dynamicAlloc_t.
    allocPointer_t fileHandle,
    // Offset of first byte to read.
    int32_t pos,
    int32_t amount
);
// Retrieves a list of all file names in the system volume. The output stores an array of pointers to dynamic allocations.
allocPointer_t getAllFileNames();

// Opens a file with the given name. Uses openFile for underlying logic.
// "stringAlloc" is a pointer to a dynamicAlloc_t containing the name of the file to open.
// Returns a pointer to a fileHandle_t.
allocPointer_t openFileByStringAlloc(allocPointer_t stringAlloc);
// Retrieves whether the given heap allocation is a file handle.
int8_t allocIsFileHandle(allocPointer_t pointer);
// Verifies whether the given pointer references a valid file handle. May assign a new value to unhandledErrorCode.
// "fileHandle" is a pointer to a fileHandle_t.
void validateFileHandle(allocPointer_t fileHandle);

// Implements the "init" WheatSystem function.
void initializeTermApp();
// Implements the "listenTerm" WheatSystem function.
void setTermObserver();
// Implements the "termSize" WheatSystem function.
void getTermSize();
// Implements the "wrtTerm" WheatSystem function.
void writeTermText();


