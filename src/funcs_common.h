
#define throw(errorCode) \
    unhandledErrorCode = errorCode; \
    return;

// Retrieves the number of elements in the given array.
#define getArrayLength(name) (int32_t)(sizeof(name) / sizeof(*name))
#define getArrayElementOffset(name, index) (index * sizeof(*name))
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

// Retrieves a member of the storage space header.
// "memberName" is the name of a member in storageSpaceHeader_t.
#define getStorageSpaceMember(memberName) readStorageSpace(getStructMemberOffset(storageSpaceHeader_t, memberName), getStructMemberType(storageSpaceHeader_t, memberName))
// Modifies a member of the storage space header.
// "memberName" is the name of a member in storageSpaceHeader_t.
#define setStorageSpaceMember(memberName, value) writeStorageSpace(getStructMemberOffset(storageSpaceHeader_t, memberName), getStructMemberType(storageSpaceHeader_t, memberName), value)

// Retrieves the address of the first file in the linked list.
#define getFirstFileAddress() getStorageSpaceMember(firstFileAddress)

// Retrieves a member of the file header at the given storage address.
// "memberName" is the name of a member in fileHeader_t.
#define getFileHeaderMember(address, memberName) \
    readStorageSpace(address + getStructMemberOffset(fileHeader_t, memberName), getStructMemberType(fileHeader_t, memberName))
// Modifies a member of the file header at the given storage address.
// "memberName" is the name of a member in fileHeader_t.
#define setFileHeaderMember(address, memberName, value) \
    writeStorageSpace(address + getStructMemberOffset(fileHeader_t, memberName), getStructMemberType(fileHeader_t, memberName), value)

// Retrieves the name address of the given file.
#define getFileNameAddress(fileAddress) (fileAddress + sizeof(fileHeader_t))
// Retrieves the start address of the data region in the given file.
#define getFileDataAddress(fileAddress, nameSize) \
    (getFileNameAddress(fileAddress) + nameSize)
// Retrieves the total amount of storage space which a file occupies.
#define getFileStorageSize(nameSize, contentSize) \
    (sizeof(fileHeader_t) + nameSize + contentSize)

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
// Retrieves the start address of the data region in the given file.
#define getFileHandleDataAddress(fileHandle) \
    getFileDataAddress(getFileHandleMember(fileHandle, address), getFileHandleMember(fileHandle, nameSize))

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
// Throws an error if the given file range is out of bounds.
// "pos" is the offset of first byte in the range.
// "size" is the number of bytes in the range.
#define validateFileRange(fileHandle, pos, size) { \
    int32_t contentSize = getFileHandleSize(fileHandle); \
    if (pos < 0 || pos >= contentSize) { \
        throw(INDEX_ERR_CODE); \
    } \
    if (size < 0 || pos + size > contentSize) { \
        throw(NUM_RANGE_ERR_CODE); \
    } \
}

// Retrieves whether a file has admin permission from the given bit field.
#define getHasAdminPermFromFileAttributes(fileAttributes) \
    ((fileAttributes & ADMIN_FILE_ATTR) > 0)
// Retrieves whether a file is guarded from the given bit field.
#define getIsGuardedFromFileAttributes(fileAttributes) \
    ((fileAttributes & GUARDED_FILE_ATTR) > 0)
// Retrieves the type of a file from the given bit field.
#define getTypeFromFileAttributes(fileAttributes) (fileAttributes & 0x03)

// Retrieves a member of the running application header in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in runningAppHeader_t.
#define getRunningAppMember(runningApp, memberName) \
    readAlloc(runningApp, getStructMemberOffset(runningAppHeader_t, memberName), getStructMemberType(runningAppHeader_t, memberName))
// Modifies a member of the running application header in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in runningAppHeader_t.
#define setRunningAppMember(runningApp, memberName, value) \
    writeAlloc(runningApp, getStructMemberOffset(runningAppHeader_t, memberName), getStructMemberType(runningAppHeader_t, memberName), value)

// Retrieves the address of the global frame data region in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
#define getGlobalFrameDataAddress(runningApp) \
    (getAllocDataAddress(runningApp) + sizeof(runningAppHeader_t))
// Retrieves the size of the global frame data region in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
#define getGlobalFrameSize(runningApp) \
    (getAllocSize(runningApp) - sizeof(runningAppHeader_t))

// Reads a value from the global frame data region in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the offset of first byte to read.
#define readGlobalFrame(runningApp, index, type) \
    readHeapMemory(getGlobalFrameDataAddress(runningApp) + index, type)
// Writes a value to the global frame data region in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the offset of first byte to write.
#define writeGlobalFrame(runningApp, index, type, value) \
    writeHeapMemory(getGlobalFrameDataAddress(runningApp) + index, type, value)

// Retrieves a member of the local frame header in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "memberName" is the name of a member in localFrameHeader_t.
#define getLocalFrameMember(localFrame, memberName) \
    readAlloc(localFrame, getStructMemberOffset(localFrameHeader_t, memberName), getStructMemberType(localFrameHeader_t, memberName))
// Modifies a member of the local frame header in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "memberName" is the name of a member in localFrameHeader_t.
#define setLocalFrameMember(localFrame, memberName, value) \
    writeAlloc(localFrame, getStructMemberOffset(localFrameHeader_t, memberName), getStructMemberType(localFrameHeader_t, memberName), value)

// Retrieves the address of the data region in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
#define getLocalFrameDataAddress(localFrame) \
    (getAllocDataAddress(localFrame) + sizeof(localFrameHeader_t))
// Retrieves the size of the data region in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
#define getLocalFrameSize(localFrame) \
    (getAllocSize(localFrame) - sizeof(localFrameHeader_t))

// Reads a value from the data region in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "index" is the offset of first byte to read.
#define readLocalFrame(localFrame, index, type) \
    readHeapMemory(getLocalFrameDataAddress(localFrame) + index, type)
// Writes a value to the data region in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "index" is the offset of first byte to write.
#define writeLocalFrame(localFrame, index, type, value) \
    writeHeapMemory(getLocalFrameDataAddress(localFrame) + index, type, value)

// Retrieves the address of the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
#define getArgFrameDataAddress(argFrame) getAllocDataAddress(argFrame)
// Retrieves the size of the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
#define getArgFrameSize(argFrame) getAllocSize(argFrame)

// Reads a value from the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
// "index" is the offset of first byte to read.
#define readArgFrame(argFrame, index, type) \
    readAlloc(argFrame, index, type)
// Writes a value to the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
// "index" is the offset of first byte to write.
#define writeArgFrame(argFrame, index, type, value) \
    writeAlloc(argFrame, index, type, value)

// Retrieves the argument frame which has been passed to the current function invocation.
#define getPreviousArgFrame() ({ \
    allocPointer_t tempLocalFrame = getLocalFrameMember(currentLocalFrame, previousLocalFrame); \
    getLocalFrameMember(tempLocalFrame, nextArgFrame); \
})
// Deletes any argument frame which has been created by the current function invocation.
#define cleanUpNextArgFrame() cleanUpNextArgFrameHelper(currentLocalFrame)

// Retrieves a member from the header of the given bytecode application.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "memberName" is the name of a member in bytecodeAppHeader_t
#define getBytecodeAppMember(fileHandle, memberName) \
    readFile(fileHandle, getStructMemberOffset(bytecodeAppHeader_t, memberName), getStructMemberType(bytecodeAppHeader_t, memberName))

// Reads a value from the function table of a bytecode app.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "index" is the offset of first byte to read.
#define readBytecodeFunctionTable(fileHandle, index, type) \
    readFile(fileHandle, sizeof(bytecodeAppHeader_t) + index, type)

// Retrieves a member from a function in the given bytecode application.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "memberName" is the name of a member in bytecodeFunction_t
#define getBytecodeFunctionMember(fileHandle, functionIndex, memberName) \
    readBytecodeFunctionTable(fileHandle, functionIndex * sizeof(bytecodeFunction_t) + getStructMemberOffset(bytecodeFunction_t, memberName), getStructMemberType(bytecodeFunction_t, memberName))

// Retrieves a member from the global frame header belonging to the given bytecode application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in bytecodeGlobalFrameHeader_t.
#define getBytecodeGlobalFrameMember(runningApp, memberName) \
    readGlobalFrame(runningApp, getStructMemberOffset(bytecodeGlobalFrameHeader_t, memberName), getStructMemberType(bytecodeGlobalFrameHeader_t, memberName))
// Modifies a member in the global frame header belonging to the given bytecode application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in bytecodeGlobalFrameHeader_t.
#define setBytecodeGlobalFrameMember(runningApp, memberName, value) \
    writeGlobalFrame(runningApp, getStructMemberOffset(bytecodeGlobalFrameHeader_t, memberName), getStructMemberType(bytecodeGlobalFrameHeader_t, memberName), value)

// Retrieves the address of the global frame data region belonging to the given bytecode application.
// "runningApp" is an allocPointer_t to a runningApp_t.
#define getBytecodeGlobalFrameDataAddress(runningApp) \
    (getGlobalFrameDataAddress(runningApp) + sizeof(bytecodeGlobalFrameHeader_t))
// Retrieves the size of the global frame data region belonging to the given bytecode application.
// "runningApp" is an allocPointer_t to a runningApp_t.
#define getBytecodeGlobalFrameSize(runningApp) \
    (getGlobalFrameSize(runningApp) - sizeof(bytecodeGlobalFrameHeader_t))

// Retrieves a member from the header of the given bytecode local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "memberName" is the name of a member in bytecodeLocalFrameHeader_t.
#define getBytecodeLocalFrameMember(localFrame, memberName) \
    readLocalFrame(localFrame, getStructMemberOffset(bytecodeLocalFrameHeader_t, memberName), getStructMemberType(bytecodeLocalFrameHeader_t, memberName))
// Modifies a member in the header of the given bytecode local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "memberName" is the name of a member in bytecodeLocalFrameHeader_t.
#define setBytecodeLocalFrameMember(localFrame, memberName, value) \
    writeLocalFrame(localFrame, getStructMemberOffset(bytecodeLocalFrameHeader_t, memberName), getStructMemberType(bytecodeLocalFrameHeader_t, memberName), value)

// Retrieves the address of the data region in the given bytecode local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
#define getBytecodeLocalFrameDataAddress(localFrame) \
    (getLocalFrameDataAddress(localFrame) + sizeof(bytecodeLocalFrameHeader_t))
// Retrieves the size of the data region in the given bytecode local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
#define getBytecodeLocalFrameSize(localFrame) \
    (getLocalFrameSize(localFrame) - sizeof(bytecodeLocalFrameHeader_t))

// Reads a value at currentInstructionFilePos within the current bytecode application.
#define readInstructionData(type) ({ \
    if (currentInstructionFilePos < instructionBodyStartFilePos || currentInstructionFilePos + sizeof(type) > instructionBodyEndFilePos) { \
        throw(INDEX_ERR_CODE); \
    } \
    type result = readFile(currentImplementerFileHandle, currentInstructionFilePos, type); \
    currentInstructionFilePos += sizeof(type); \
    result; \
})

// Macros to read attributes in an bytecode instruction argument prefix.
#define getArgPrefixReferenceType(argPrefix) (argPrefix >> 4)
#define getArgPrefixDataType(argPrefix) (argPrefix & 0x0F)

// Retrieves the number of bytes which the given argument type occupies.
#define getArgDataTypeSize(dataType) (dataType == SIGNED_INT_8_TYPE ? 1 : 4)

// Reads an integer from a bytecode argument.
#define readArgInt(index) ({ \
    int32_t tempResult = readArgIntHelper(instructionArgArray + index, 0, -1); \
    if (unhandledErrorCode != 0) { \
        return; \
    } \
    tempResult; \
})
// Writes an integer to a bytecode argument.
#define writeArgInt(index, value) \
    writeArgIntHelper(instructionArgArray + index, 0, -1, value); \
    if (unhandledErrorCode != 0) { \
        return; \
    }

// Reads an integer from a bytecode argument, and enforces that the integer is constant.
#define readArgConstantInt(index) ({ \
    int32_t tempResult = readArgConstantIntHelper(index); \
    if (unhandledErrorCode != 0) { \
        return; \
    } \
    tempResult; \
});
// Reads a pointer to a dynamic allocation from a bytecode argument.
#define readArgDynamicAlloc(index) ({ \
    allocPointer_t pointer = readArgInt(index); \
    validateDynamicAlloc(pointer); \
    pointer; \
})

// Reads a pointer to a file handle from a bytecode argument.
#define readArgFileHandle(index) ({ \
    allocPointer_t fileHandle = readArgInt(index); \
    validateFileHandle(fileHandle); \
    fileHandle; \
});
// Reads a pointer to a running app from a bytecode argument.
#define readArgRunningApp(index) ({ \
    allocPointer_t runningApp; \
    readArgRunningAppHelper(&runningApp, index); \
    runningApp; \
});

// Convenience function to create a system application.
// "systemAppFunctionArray" is a fixed array of systemAppFunction_t.
#define createSystemApp(globalFrameSize, systemAppFunctionArray) \
    (systemApp_t){globalFrameSize, systemAppFunctionArray, getArrayLength(systemAppFunctionArray)}

// Retrieves a member from the system application with the given ID.
// "memberName" is the name of a member in systemApp_t.
#define getSystemAppMember(id, memberName) \
    readFixedArrayValue(systemAppArray, id * sizeof(systemApp_t) + getStructMemberOffset(systemApp_t, memberName), getStructMemberType(systemApp_t, memberName))
// "systemAppFunctionArray" is a fixed array of systemAppFunction_t.
// "index" is the index of function in "systemAppFunctionArray".
// "memberName" is the name of a member in systemAppFunction_t.
#define getSystemAppFunctionListMember(systemAppFunctionArray, index, memberName) \
    readFixedArrayValue(systemAppFunctionArray, index * sizeof(systemAppFunction_t) + getStructMemberOffset(systemAppFunction_t, memberName), getStructMemberType(systemAppFunction_t, memberName))
// Retrieves a member from a function belonging to the system application with the given ID.
// "index" is the index of function in functionList of systemApp_t.
// "memberName" is the name of a member in systemAppFunction_t.
#define getSystemAppFunctionMember(id, index, memberName) ({ \
    const systemAppFunction_t *functionList = getSystemAppMember(id, functionList); \
    getSystemAppFunctionListMember(functionList, index, memberName); \
})

// Retrieves a member from the global frame header belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in systemGlobalFrameHeader_t.
#define getSystemGlobalFrameMember(runningApp, memberName) \
    readGlobalFrame(runningApp, getStructMemberOffset(systemGlobalFrameHeader_t, memberName), getStructMemberType(systemGlobalFrameHeader_t, memberName))
// Modifies a member in the global frame header belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in systemGlobalFrameHeader_t.
#define setSystemGlobalFrameMember(runningApp, memberName, value) \
    writeGlobalFrame(runningApp, getStructMemberOffset(systemGlobalFrameHeader_t, memberName), getStructMemberType(systemGlobalFrameHeader_t, memberName), value)

// Retrieves the address of the global frame data region belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
#define getSystemGlobalFrameDataAddress(runningApp) \
    (getGlobalFrameDataAddress(runningApp) + sizeof(systemGlobalFrameHeader_t))

// Reads a value from the global frame data region belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the offset of first byte to read.
#define readSystemGlobalFrame(runningApp, index, type) \
    readHeapMemory(getSystemGlobalFrameDataAddress(runningApp) + index, type)
// Writes a value to the global frame data region belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the offset of first byte to write.
#define writeSystemGlobalFrame(runningApp, index, type, value) \
    writeHeapMemory(getSystemGlobalFrameDataAddress(runningApp) + index, type, value)

// Reads a global variable of the currently active system application.
// "structDefinition" is the structure of global frame data region.
// "memberName" is the name of a member in "structDefinition".
#define readSystemAppGlobalVariable(structDefinition, memberName) \
    readSystemGlobalFrame(currentImplementer, getStructMemberOffset(structDefinition, memberName), getStructMemberType(structDefinition, memberName))
// Writes a global variable of the currently active system application.
// "structDefinition" is the structure of global frame data region.
// "memberName" is the name of a member in "structDefinition".
#define writeSystemAppGlobalVariable(structDefinition, memberName, value) \
    writeSystemGlobalFrame(currentImplementer, getStructMemberOffset(structDefinition, memberName), getStructMemberType(structDefinition, memberName), value)

// Retrieves a member from the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "memberName" is the name of a member in systemApp_t.
#define getRunningSystemAppMember(runningApp, memberName) ({ \
    int8_t systemAppId = getSystemGlobalFrameMember(runningApp, id); \
    getSystemAppMember(systemAppId, memberName); \
})
// Retrieves a member from a function belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the index of function in functionList of systemApp_t.
// "memberName" is the name of a member in systemAppFunction_t.
#define getRunningSystemAppFunctionMember(runningApp, functionIndex, memberName) ({ \
    int8_t systemAppId = getSystemGlobalFrameMember(runningApp, id); \
    getSystemAppFunctionMember(systemAppId, functionIndex, memberName); \
})

// Retrieves a global variable of a term driver.
// "memberName" is the name of a member in termAppGlobalFrame_t.
#define readTermAppGlobalVariable(memberName) \
    readSystemAppGlobalVariable(termAppGlobalFrame_t, memberName)
// Modifies a global variable of a term driver.
// "memberName" is the name of a member in termAppGlobalFrame_t.
#define writeTermAppGlobalVariable(memberName, value) \
    writeSystemAppGlobalVariable(termAppGlobalFrame_t, memberName, value)

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

// Creates a file with the given name and attributes.
// "name" is a pointer to a dynamicAlloc_t.
void createFile(allocPointer_t name, int8_t type, int8_t isGuarded, int32_t contentSize);
// Deletes the given file.
// "fileHandle" is a pointer to a dynamicAlloc_t.
void deleteFile(allocPointer_t fileHandle);
// Finds the address of the file with the given name. Returns MISSING_FILE_ADDRESS if the file cannot be found.
int32_t getFileAddressByName(heapMemoryOffset_t nameAddress, heapMemoryOffset_t nameSize);
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

// Retrieves an index in the function table of the given running application.
// "runningApp" is a pointer to a runningApp_t.
int32_t findFunctionById(allocPointer_t runningApp, int32_t functionId);
// Retrieves the running application which implements the caller of the currently active function invocation.
// Returns a pointer to a runningApp_t.
allocPointer_t getCurrentCaller();
// Creates an argument frame to be passed to the next function invocation.
// "size" is the size of data region in the new argument frame.
// Returns a pointer to a argFrame_t.
allocPointer_t createNextArgFrame(heapMemoryOffset_t size);
// "localFrame" is a pointer to a localFrame_t.
void cleanUpNextArgFrameHelper(allocPointer_t localFrame);
// Launches a running application from the given file handle.
// "fileHandle" is a pointer to a fileHandle_t.
void launchApp(allocPointer_t fileHandle);
// Invokes a function in the given thread.
// "threadApp" and "implementer" are pointers to runningApp_t.
// "functionIndex" is the index of the function in "implementer".
void callFunction(
    allocPointer_t threadApp,
    allocPointer_t implementer,
    int32_t functionIndex
);
// Stops evaluation of the current function invocation, and returns control to the previous function invocation.
void returnFromFunction();
// Enters a blocking loop to run all WheatSystem applications.
void runAppSystem();

// Determines the address of a bytecode argument which references a heap allocation.
heapMemoryOffset_t getArgHeapMemoryAddress(
    instructionArg_t *arg,
    int32_t offset,
    int8_t dataTypeSize
);
// Determines the size of the data interval which is referenced by the given bytecode argument.
int32_t getArgBufferSize(instructionArg_t *arg);

// Helper functions for corresponding macros.
int32_t readArgIntHelper(instructionArg_t *arg, int32_t offset, int8_t dataType);
void writeArgIntHelper(
    instructionArg_t *arg,
    int32_t offset,
    int8_t dataType,
    int32_t value
);
int32_t readArgConstantIntHelper(int8_t index);
void readArgRunningAppHelper(allocPointer_t *destination, int8_t index);

// Causes the bytecode interpreter to jump to the given position in the current function.
// "instructionOffset" is the offset from the beginning of the current function body.
void jumpToBytecodeInstruction(int32_t instructionOffset);
void parseInstructionArg(instructionArg_t *destination);
// Interprets one bytecode instruction of the currently scheduled bytecode application.
void evaluateBytecodeInstruction();

// Implements the "init" WheatSystem function.
void initializeTermApp();
// Implements the "listenTerm" WheatSystem function.
void setTermObserver();
// Implements the "termSize" WheatSystem function.
void getTermSize();
// Implements the "wrtTerm" WheatSystem function.
void writeTermText();


