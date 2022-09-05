
#define throw(errorCode, ...) \
    unhandledErrorCode = errorCode; \
    return __VA_ARGS__;
#define checkUnhandledError(...) \
    if (unhandledErrorCode != NONE_ERR_CODE) { \
        return __VA_ARGS__; \
    }

#define throwInSystemApp(errorCode) \
    unhandledErrorCode = errorCode; \
    returnFromFunc(); \
    return;
#define checkErrorInSystemApp() \
    if (unhandledErrorCode != NONE_ERR_CODE) { \
        returnFromFunc(); \
        return; \
    }

#define getMaximum(value1, value2) ((value1 > value2) ? value1 : value2)

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

// Retrieves the address of a span header member in the given span.
// "address" is the start address of the span.
// "memberName" is the name of a member in spanHeader_t.
#define getSpanMemberAddress(address, memberName) \
    address + getStructMemberOffset(spanHeader_t, memberName)
// Retrieves a member of the span header in the given span.
// "address" is the start address of the span.
// "memberName" is the name of a member in spanHeader_t.
#define getSpanMember(address, memberName) \
    readHeapMem(getSpanMemberAddress(address, memberName), getStructMemberType(spanHeader_t, memberName))
// Modifies a member of the span header in the given span.
// "address" is the start address of the span.
// "memberName" is the name of a member in spanHeader_t.
#define setSpanMember(address, memberName, value) \
    writeHeapMem(getSpanMemberAddress(address, memberName), getStructMemberType(spanHeader_t, memberName), value)

// Retrieves the start address of the data region in the given span.
// "address" is the start address of the span.
#define getSpanDataAddress(address) (address + sizeof(spanHeader_t))
// Reads a value from the data region in the given span.
// "address" is the start address of the span.
// "index" is the offset of value in the data region.
#define readSpan(address, index, type) \
    readHeapMem(getSpanDataAddress(address) + index, type)
// Write a value to the data region in the given span.
// "address" is the start address of the span.
// "index" is the offset of value in the data region.
#define writeSpan(address, index, type, value) \
    writeHeapMem(getSpanDataAddress(address) + index, type, value)

// Retrieves a member of the empty span header in the given span.
// "address" is the start address of the span.
// "memberName" is the name of a member in emptySpanHeader_t.
#define getEmptySpanMember(address, memberName) \
    readSpan(address, getStructMemberOffset(emptySpanHeader_t, memberName), getStructMemberType(emptySpanHeader_t, memberName))
// Modifies a member of the empty span header in the given span.
// "address" is the start address of the span.
// "memberName" is the name of a member in emptySpanHeader_t.
#define setEmptySpanMember(address, memberName, value) \
    writeSpan(address, getStructMemberOffset(emptySpanHeader_t, memberName), getStructMemberType(emptySpanHeader_t, memberName), value)

// Converts the start address of a span to the allocPointer_t of its child allocation.
#define getSpanAllocPointer(address) getSpanDataAddress(address)
// Converts the allocPointer_t of an allocation to the start address of its parent span.
#define getAllocSpanAddress(pointer) (pointer - sizeof(spanHeader_t))
// Retrieves the amount of heap memory required to store the given allocation.
#define getAllocMemUsage(pointer) ({ \
    heapMemOffset_t address = getAllocSpanAddress(pointer); \
    getSpanMember(address, size) + sizeof(spanHeader_t); \
})

// Retrieves the address of an allocation header member in the given allocation.
// "pointer" is an allocPointer_t.
// "memberName" is the name of a member in allocHeader_t.
#define getAllocMemberAddress(pointer, memberName) \
    pointer + getStructMemberOffset(allocHeader_t, memberName)
// Retrieves a member of the allocation header in the given allocation.
// "pointer" is an allocPointer_t.
// "memberName" is the name of a member in allocHeader_t.
#define getAllocMember(pointer, memberName) \
    readHeapMem(getAllocMemberAddress(pointer, memberName), getStructMemberType(allocHeader_t, memberName))
// Modifies a member of the allocation header in the given allocation.
// "pointer" is an allocPointer_t.
// "memberName" is the name of a member in allocHeader_t.
#define setAllocMember(pointer, memberName, value) \
    writeHeapMem(getAllocMemberAddress(pointer, memberName), getStructMemberType(allocHeader_t, memberName), value)

// Retrieves the type of the given allocation.
#define getAllocType(pointer) getSpanMember(getAllocSpanAddress(pointer), allocType)
// Retrieves a pointer to the next allocation which has the same type.
#define getAllocNextByType(pointer) getAllocMember(pointer, nextByType)
// Retrieves the size of the data region in the given allocation.
#define getAllocSize(pointer) getAllocMember(pointer, size)

// Retrieves the start address of the data region in the given allocation.
#define getAllocDataAddress(pointer) (pointer + sizeof(allocHeader_t))
// Reads a value from the data region in the given allocation.
// "pointer" is an allocPointer_t.
// "index" is the offset of value in the data region.
#define readAlloc(pointer, index, type) \
    readHeapMem(getAllocDataAddress(pointer) + index, type)
// Write a value to the data region in the given allocation.
// "pointer" is an allocPointer_t.
// "index" is the offset of value in the data region.
#define writeAlloc(pointer, index, type, value) \
    writeHeapMem(getAllocDataAddress(pointer) + index, type, value)

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
    readHeapMem(getDynamicAllocDataAddress(pointer) + index, type)
// Writes a value to the data region of the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
// "index" is the integer offset of first byte to write.
#define writeDynamicAlloc(pointer, index, type, value) \
    writeHeapMem(getDynamicAllocDataAddress(pointer) + index, type, value)

// Retrieves the size of the data region in the given dynamic allocation.
// "pointer" is an allocPointer_t to a dynamicAlloc_t.
#define getDynamicAllocSize(pointer) \
    (getAllocSize(pointer) - sizeof(dynamicAllocHeader_t))

// Creates a dynamic allocation whose data region contains the string in the given fixed array.
// "fixedArray" is a fixed array of int8_t ending in the null character.
#define createStringAllocFromFixedArray(fixedArray) \
    createStringAllocFromFixedArrayHelper(fixedArray, (heapMemOffset_t)(getArrayLength(fixedArray) - 1))

// Retrieves a member of the system sentry header in the given system sentry.
// "pointer" is an allocPointer_t to a systemSentry_t.
// "memberName" is the name of a member in systemSentryHeader_t.
#define getSystemSentryMember(pointer, memberName) \
    readDynamicAlloc(pointer, getStructMemberOffset(systemSentryHeader_t, memberName), getStructMemberType(systemSentryHeader_t, memberName))
// Modifies a member of the system sentry header in the given system sentry.
// "pointer" is an allocPointer_t to a systemSentry_t.
// "memberName" is the name of a member in systemSentryHeader_t.
#define setSystemSentryMember(pointer, memberName, value) \
    writeDynamicAlloc(pointer, getStructMemberOffset(systemSentryHeader_t, memberName), getStructMemberType(systemSentryHeader_t, memberName), value)

// Retrieves the address of the data region in the given system sentry.
// "pointer" is an allocPointer_t to a systemSentry_t.
#define getSystemSentryDataAddress(pointer) \
    (getDynamicAllocDataAddress(pointer) + sizeof(systemSentryHeader_t))
// Reads a value from the data region of the given system sentry.
// "pointer" is an allocPointer_t to a systemSentry_t.
// "index" is the integer offset of first byte to read.
#define readSystemSentry(pointer, index, type) \
    readHeapMem(getSystemSentryDataAddress(pointer) + index, type)
// Writes a value to the data region of the given system sentry.
// "pointer" is an allocPointer_t to a systemSentry_t.
// "index" is the integer offset of first byte to write.
#define writeSystemSentry(pointer, index, type, value) \
    writeHeapMem(getSystemSentryDataAddress(pointer) + index, type, value)

// Retrieves a member of the given gate sentry.
// "gate" is an allocPointer_t to a gateSentry_t.
// "memberName" is the name of a member in gateSentry_t.
#define getGateMember(gate, memberName) \
    readSystemSentry(gate, getStructMemberOffset(gateSentry_t, memberName), getStructMemberType(gateSentry_t, memberName))
// Modifies a member of the given gate sentry.
// "gate" is an allocPointer_t to a gateSentry_t.
// "memberName" is the name of a member in gateSentry_t.
#define setGateMember(gate, memberName, value) \
    writeSystemSentry(gate, getStructMemberOffset(gateSentry_t, memberName), getStructMemberType(gateSentry_t, memberName), value)
// Changes the state of the given gate to be closed.
// "gate" is an allocPointer_t to a gateSentry_t.
#define closeGate(gate) setGateMember(gate, isOpen, false)

// Reads a value from non-volatile storage.
// "address" is the offset of first byte to read.
#define readStorage(address, type) \
    ({type result; readStorageRange(&result, address, sizeof(type)); result;})
// Writes a value to non-volatile storage. Changes might not be persisted until calling flushStorage.
// "address" is the offset of first byte to write.
#define writeStorage(address, type, value) \
    ({type tempValue = value; writeStorageRange(address, &tempValue, sizeof(type));})

// Retrieves a member of the storage space header.
// "memberName" is the name of a member in storageHeader_t.
#define getStorageMember(memberName) readStorage(getStructMemberOffset(storageHeader_t, memberName), getStructMemberType(storageHeader_t, memberName))
// Modifies a member of the storage space header.
// "memberName" is the name of a member in storageHeader_t.
#define setStorageMember(memberName, value) writeStorage(getStructMemberOffset(storageHeader_t, memberName), getStructMemberType(storageHeader_t, memberName), value)

// Retrieves the address of the first file in the linked list.
#define getFirstFileAddress() getStorageMember(firstFileAddress)

// Retrieves a member of the file header at the given storage address.
// "memberName" is the name of a member in fileHeader_t.
#define getFileHeaderMember(address, memberName) \
    readStorage(address + getStructMemberOffset(fileHeader_t, memberName), getStructMemberType(fileHeader_t, memberName))
// Modifies a member of the file header at the given storage address.
// "memberName" is the name of a member in fileHeader_t.
#define setFileHeaderMember(address, memberName, value) \
    writeStorage(address + getStructMemberOffset(fileHeader_t, memberName), getStructMemberType(fileHeader_t, memberName), value)

// Retrieves the name address of the given file.
#define getFileNameAddress(fileAddress) (fileAddress + sizeof(fileHeader_t))
// Retrieves the start address of the data region in the given file.
#define getFileDataAddress(fileAddress, nameSize) \
    (getFileNameAddress(fileAddress) + nameSize)
// Retrieves the total amount of storage space which a file occupies.
#define getFileStorageSizeHelper(nameSize, contentSize) \
    (sizeof(fileHeader_t) + nameSize + contentSize)

// Retrieves a member of the given file handle.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "memberName" is the name of a member in fileHandle_t.
#define getFileHandleMember(fileHandle, memberName) \
    readSystemSentry(fileHandle, getStructMemberOffset(fileHandle_t, memberName), getStructMemberType(fileHandle_t, memberName))
// Modifies a member of the given file handle.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "memberName" is the name of a member in fileHandle_t.
#define setFileHandleMember(fileHandle, memberName, value) \
    writeSystemSentry(fileHandle, getStructMemberOffset(fileHandle_t, memberName), getStructMemberType(fileHandle_t, memberName), value)

// Retrieves the type of the given file.
#define getFileHandleType(fileHandle) \
    getTypeFromFileAttributes(getFileHandleMember(fileHandle, attributes))
// Retrieves the running app of the given file. Returns NULL_ALLOC_POINTER if the file has no associated running app.
#define getFileHandleRunningApp(fileHandle) getFileHandleMember(fileHandle, runningApp)
// Retrieves the start address of the data region in the given file.
#define getFileHandleDataAddress(fileHandle) \
    getFileDataAddress(getFileHandleMember(fileHandle, address), getFileHandleMember(fileHandle, nameSize))

// Reads a value from the data region in the given file.
// "pos" is the offset of first byte to read.
#define readFile(fileHandle, pos, type) \
    ({type result; readFileRange(&result, fileHandle, pos, sizeof(type)); result;})
// Throws an error if the given file range is out of bounds.
// "pos" is the offset of first byte in the range.
// "size" is the number of bytes in the range.
#define validateFileRange(fileHandle, pos, size) { \
    storageOffset_t contentSize = getFileHandleMember(fileHandle, contentSize); \
    if (pos < 0 || pos >= contentSize) { \
        throw(INDEX_ERR_CODE); \
    } \
    if (size < 0 || pos + size > contentSize) { \
        throw(LEN_ERR_CODE); \
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

// Returns whether the implementer of the current function has admin permission.
#define currentImplementerHasAdminPerm() \
    getHasAdminPermFromFileAttributes(getFileHandleMember(currentImplementerFileHandle, attributes))
// Helper macro for runningAppMayAccessAlloc and currentImplementerMayAccessAlloc.
#define runningAppMayAccessAllocHelper(runningApp, dynamicAlloc) { \
    int8_t attributes = getDynamicAllocMember(dynamicAlloc, attributes); \
    if ((attributes & GUARDED_ALLOC_ATTR) == 0) { \
        return true; \
    } \
    allocPointer_t creator = getDynamicAllocMember(dynamicAlloc, creator); \
    if (creator == runningApp) { \
        return true; \
    } \
}

// Retrieves a member of the given thread.
// "thread" is an allocPointer_t to a thread_t.
// "memberName" is the name of a member in thread_t.
#define getThreadMember(thread, memberName) \
    readAlloc(thread, getStructMemberOffset(thread_t, memberName), getStructMemberType(thread_t, memberName))
// Modifies a member of the given thread.
// "thread" is an allocPointer_t to a thread_t.
// "memberName" is the name of a member in thread_t.
#define setThreadMember(thread, memberName, value) \
    writeAlloc(thread, getStructMemberOffset(thread_t, memberName), getStructMemberType(thread_t, memberName), value)

// Sets the currently active thread.
// "thread" is an allocPointer_t to a thread_t.
#define setCurrentThread(thread) \
    currentThread = thread; \
    currentLocalFrame = getThreadMember(currentThread, localFrame); \
    updateLocalFrameContext();
// Sets the next thread to be scheduled.
// "previousThread" is an allocPointer_t to a thread_t.
#define advanceNextThread(previousThread) \
    nextThread = getAllocNextByType(previousThread); \
    if (nextThread == NULL_ALLOC_POINTER) { \
        nextThread = allocsByType[THREAD_ALLOC_TYPE]; \
    }

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
    readHeapMem(getGlobalFrameDataAddress(runningApp) + index, type)
// Writes a value to the global frame data region in the given running application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the offset of first byte to write.
#define writeGlobalFrame(runningApp, index, type, value) \
    writeHeapMem(getGlobalFrameDataAddress(runningApp) + index, type, value)

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
    readHeapMem(getLocalFrameDataAddress(localFrame) + index, type)
// Writes a value to the data region in the given local frame.
// "localFrame" is an allocPointer_t to a localFrame_t.
// "index" is the offset of first byte to write.
#define writeLocalFrame(localFrame, index, type, value) \
    writeHeapMem(getLocalFrameDataAddress(localFrame) + index, type, value)

// Retrieves a member of the argument frame header in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
// "memberName" is the name of a member in argFrameHeader_t.
#define getArgFrameMember(argFrame, memberName) \
    readAlloc(argFrame, getStructMemberOffset(argFrameHeader_t, memberName), getStructMemberType(argFrameHeader_t, memberName))
// Modifies a member of the argument frame header in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
// "memberName" is the name of a member in argFrameHeader_t.
#define setArgFrameMember(argFrame, memberName, value) \
    writeAlloc(argFrame, getStructMemberOffset(argFrameHeader_t, memberName), getStructMemberType(argFrameHeader_t, memberName), value)

// Retrieves the address of the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
#define getArgFrameDataAddress(argFrame) \
    (getAllocDataAddress(argFrame) + sizeof(argFrameHeader_t))
// Retrieves the size of the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
#define getArgFrameSize(argFrame) \
    (getAllocSize(argFrame) - sizeof(argFrameHeader_t))

// Reads a value from the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
// "index" is the offset of first byte to read.
#define readArgFrame(argFrame, index, type) \
    readHeapMem(getArgFrameDataAddress(argFrame) + index, type)
// Writes a value to the data region in the given argument frame.
// "argFrame" is an allocPointer_t to an argFrame_t.
// "index" is the offset of first byte to write.
#define writeArgFrame(argFrame, index, type, value) \
    writeHeapMem(getArgFrameDataAddress(argFrame) + index, type, value)

// Retrieves the argument frame which has been passed to the current function invocation.
#define getPreviousArgFrame() getLocalFrameMember(currentLocalFrame, previousArgFrame)
// Deletes any argument frame which has been created by the current function invocation.
#define cleanUpNextArgFrame() cleanUpNextArgFrameHelper(currentLocalFrame)

// Helper macros coupled tightly to callFunc and funcIsGuarded.
#define validateFuncIndex(implementer, fileType, funcIndex, systemAppFuncList, ...) { \
    int32_t funcAmount; \
    if (fileType == BYTECODE_APP_FILE_TYPE) { \
        funcAmount = getBytecodeGlobalFrameMember(implementer, funcTableLength); \
    } else { \
        int8_t systemAppId = getSystemGlobalFrameMember(implementer, id); \
        systemAppFuncList = getSystemAppMember(systemAppId, funcList); \
        funcAmount = getSystemAppMember(systemAppId, funcAmount); \
    } \
    if (funcIndex < 0 || funcIndex >= funcAmount) { \
        throw(INDEX_ERR_CODE, __VA_ARGS__); \
    } \
}
#define funcIsGuardedHelper(destination, fileHandle, fileType, funcIndex, systemAppFuncList) \
    if (fileType == BYTECODE_APP_FILE_TYPE) { \
        destination = getBytecodeFuncMember(fileHandle, funcIndex, isGuarded); \
    } else { \
        destination = getSystemAppFuncListMember( \
            systemAppFuncList, \
            funcIndex, \
            isGuarded \
        ); \
    }

// Retrieves a member from the header of the given bytecode application.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "memberName" is the name of a member in bytecodeAppHeader_t.
#define getBytecodeAppMember(fileHandle, memberName) \
    readFile(fileHandle, getStructMemberOffset(bytecodeAppHeader_t, memberName), getStructMemberType(bytecodeAppHeader_t, memberName))

// Reads a value from the function table of a bytecode app.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "index" is the offset of first byte to read.
#define readBytecodeFuncTable(fileHandle, index, type) \
    readFile(fileHandle, sizeof(bytecodeAppHeader_t) + index, type)

// Retrieves a member from a function in the given bytecode application.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
// "memberName" is the name of a member in bytecodeFunc_t
#define getBytecodeFuncMember(fileHandle, funcIndex, memberName) \
    readBytecodeFuncTable(fileHandle, funcIndex * sizeof(bytecodeFunc_t) + getStructMemberOffset(bytecodeFunc_t, memberName), getStructMemberType(bytecodeFunc_t, memberName))

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
    int32_t result = readArgIntHelper(instructionArgArray + index); \
    checkUnhandledError(); \
    result; \
})
// Writes an integer to a bytecode argument.
#define writeArgInt(index, value) \
    writeArgIntHelper(instructionArgArray + index, value); \
    checkUnhandledError();

// Reads an integer from a bytecode argument, and enforces that the integer is constant.
#define readArgConstantInt(index) ({ \
    int32_t result = readArgConstantIntHelper(index); \
    checkUnhandledError(); \
    result; \
})
// Reads a pointer to a dynamic allocation from a bytecode argument.
#define readArgDynamicAlloc(index) ({ \
    int32_t pointer = readArgInt(index); \
    validateDynamicAlloc(pointer); \
    checkUnhandledError(); \
    (allocPointer_t)pointer; \
})

// Reads a pointer to a gate sentry from a bytecode argument.
#define readArgGate(index) ({ \
    int32_t gate = readArgInt(index); \
    validateGate(gate); \
    checkUnhandledError(); \
    (allocPointer_t)gate; \
})

// Reads a pointer to a file handle from a bytecode argument.
#define readArgFileHandle(index) ({ \
    int32_t fileHandle = readArgInt(index); \
    validateSystemSentry(fileHandle, FILE_HANDLE_SENTRY_TYPE); \
    checkUnhandledError(); \
    (allocPointer_t)fileHandle; \
})
// Reads a pointer to a running app from a bytecode argument.
#define readArgRunningApp(index) ({ \
    allocPointer_t runningApp; \
    readArgRunningAppHelper(&runningApp, index); \
    checkUnhandledError(); \
    runningApp; \
})

// Convenience function to create a system application.
// "systemAppFuncArray" is a fixed array of systemAppFunc_t.
#define createSystemApp(globalFrameSize, systemAppFuncArray) \
    (systemApp_t){globalFrameSize, systemAppFuncArray, getArrayLength(systemAppFuncArray)}

// Retrieves a member from the system application with the given ID.
// "memberName" is the name of a member in systemApp_t.
#define getSystemAppMember(id, memberName) \
    readFixedArrayValue(systemAppArray, id * sizeof(systemApp_t) + getStructMemberOffset(systemApp_t, memberName), getStructMemberType(systemApp_t, memberName))
// "systemAppFuncArray" is a fixed array of systemAppFunc_t.
// "index" is the index of function in "systemAppFuncArray".
// "memberName" is the name of a member in systemAppFunc_t.
#define getSystemAppFuncListMember(systemAppFuncArray, index, memberName) \
    readFixedArrayValue(systemAppFuncArray, index * sizeof(systemAppFunc_t) + getStructMemberOffset(systemAppFunc_t, memberName), getStructMemberType(systemAppFunc_t, memberName))
// Retrieves a member from a function belonging to the system application with the given ID.
// "index" is the index of function in funcList of systemApp_t.
// "memberName" is the name of a member in systemAppFunc_t.
#define getSystemAppFuncMember(id, index, memberName) ({ \
    const systemAppFunc_t *funcList = getSystemAppMember(id, funcList); \
    getSystemAppFuncListMember(funcList, index, memberName); \
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
    readHeapMem(getSystemGlobalFrameDataAddress(runningApp) + index, type)
// Writes a value to the global frame data region belonging to the given system application.
// "runningApp" is an allocPointer_t to a runningApp_t.
// "index" is the offset of first byte to write.
#define writeSystemGlobalFrame(runningApp, index, type, value) \
    writeHeapMem(getSystemGlobalFrameDataAddress(runningApp) + index, type, value)

// Reads a global variable of the currently active system application.
// "structDefinition" is the structure of global frame data region.
// "memberName" is the name of a member in "structDefinition".
#define readSystemAppGlobalVar(structDefinition, memberName) \
    readSystemGlobalFrame(currentImplementer, getStructMemberOffset(structDefinition, memberName), getStructMemberType(structDefinition, memberName))
// Writes a global variable of the currently active system application.
// "structDefinition" is the structure of global frame data region.
// "memberName" is the name of a member in "structDefinition".
#define writeSystemAppGlobalVar(structDefinition, memberName, value) \
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
// "index" is the index of function in funcList of systemApp_t.
// "memberName" is the name of a member in systemAppFunc_t.
#define getRunningSystemAppFuncMember(runningApp, funcIndex, memberName) ({ \
    int8_t systemAppId = getSystemGlobalFrameMember(runningApp, id); \
    getSystemAppFuncMember(systemAppId, funcIndex, memberName); \
})

// Retrieves a global variable of the term driver.
// "memberName" is the name of a member in termAppGlobalFrame_t.
#define readTermAppGlobalVar(memberName) \
    readSystemAppGlobalVar(termAppGlobalFrame_t, memberName)
// Modifies a global variable of the term driver.
// "memberName" is the name of a member in termAppGlobalFrame_t.
#define writeTermAppGlobalVar(memberName, value) \
    writeSystemAppGlobalVar(termAppGlobalFrame_t, memberName, value)

// Determines the degree of the given span size.
int8_t getSpanSizeDegree(heapMemOffset_t size);
// Initializes members of emptySpanHeader_t in the given span, and adds the span to a linked list.
// "address" is the start address of the span.
// "size" is the size of the data region in the span.
void initializeEmptySpan(heapMemOffset_t address, heapMemOffset_t size);
// Removes the given span from its linked list of empty spans.
// "address" is the start address of the span.
void cleanUpEmptySpan(heapMemOffset_t address);

// Creates a heap allocation.
// "size" is the size of data region in the new allocation.
allocPointer_t createAlloc(int8_t type, heapMemOffset_t size);
// Frees the given heap allocation.
void deleteAlloc(allocPointer_t pointer);

// Verifies whether the given pointer is valid. May set unhandledErrorCode to NULL_ERR_CODE or PTR_ERR_CODE.
void validateAllocPointer(int32_t pointer);

// Returns an allocPointer_t to a dynamicAlloc_t.
allocPointer_t createDynamicAlloc(
    // Number of bytes in the data region of the new dynamic allocation.
    heapMemOffset_t size,
    int8_t attributes,
    // Pointer to runningApp_t.
    allocPointer_t creator
);
// Returns an allocPointer_t to a dynamicAlloc_t.
allocPointer_t createStringAllocFromFixedArrayHelper(
    // Fixed array of characters.
    const int8_t *fixedArray,
    heapMemOffset_t size
);
// Verifies whether the given pointer references a valid dynamic allocation. May assign a new value to unhandledErrorCode.
// "dynamicAlloc" is a pointer to dynamicAlloc_t.
void validateDynamicAlloc(int32_t dynamicAlloc);

// Creates a sentry allocation whose creator field is NULL_ALLOC_POINTER.
// "size" is the number of bytes in the data region of the new system sentry.
allocPointer_t createSystemSentry(int8_t type, heapMemOffset_t size);
// Determines whether a dynamic allocation is a system sentry with the given type.
// "dynamicAlloc" is a pointer to a dynamicAlloc_t
int8_t dynamicAllocIsSystemSentry(allocPointer_t dynamicAlloc, int8_t type);
// Verifies whether the given pointer references a valid system sentry. May assign a new value to unhandledErrorCode.
// "sentry" is a pointer to a systemSentry_t.
void validateSystemSentry(int32_t sentry, int8_t type);

// Creates a gate sentry with the given mode.
allocPointer_t createGate(int8_t mode);
// Deletes the given gate sentry.
void deleteGate(allocPointer_t gate);
// Returns true if the gate has become closed after passing.
int8_t passThroughGate(allocPointer_t gate);
// Blocks execution of the current thread if the given gate is closed.
void waitForGate(allocPointer_t gate);
// Changes the state of the given gate to be open.
void openGate(allocPointer_t gate);
// Verifies whether the given pointer references a valid gate sentry which belongs to the current implementer. May assign a new value to unhandledErrorCode.
// "sentry" is a pointer to a gateSentry_t.
void validateGate(int32_t gate);

// Determines whether a file name in heap memory equals a file name in storage.
int8_t heapMemNameEqualsStorageName(
    heapMemOffset_t heapMemNameAddress,
    uint8_t heapMemNameSize,
    storageOffset_t storageNameAddress,
    uint8_t storageNameSize
);
// Copies a file name from storage to heap memory.
void copyStorageNameToHeapMem(
    storageOffset_t storageNameAddress,
    heapMemOffset_t heapMemNameAddress,
    uint8_t nameSize
);

// Creates a file with the given name and attributes.
// "name" is a pointer to a dynamicAlloc_t.
void createFile(
    allocPointer_t name,
    int8_t type,
    int8_t isGuarded,
    storageOffset_t contentSize
);
// Deletes the given file.
// "fileHandle" is a pointer to a fileHandle_t.
void deleteFile(allocPointer_t fileHandle);
// Finds the address of the file with the given name. Returns MISSING_FILE_ADDRESS if the file cannot be found.
storageOffset_t getFileAddressByName(
    heapMemOffset_t nameAddress,
    heapMemOffset_t nameSize
);
// Determines the total amount of storage space which the given file occupies.
storageOffset_t getFileStorageSize(storageOffset_t fileAddress);
// Increments the open depth of the given file handle.
// "fileHandle" is a pointer to a fileHandle_t.
void incrementFileOpenDepth(allocPointer_t fileHandle);
// Opens the file with the given name, returning a file handle. If the file has already been opened, this function returns the existing file handle and increments its open depth. If the file is missing, this function returns NULL_ALLOC_POINTER.
allocPointer_t openFile(heapMemOffset_t nameAddress, heapMemOffset_t nameSize);
// Deletes the given file handle, and kills any running app launched from the file.
// "fileHandle" is a pointer to a fileHandle_t.
void deleteFileHandle(allocPointer_t fileHandle);
// Deletes the given file handle if it is not being used by any application.
// "fileHandle" is a pointer to a fileHandle_t.
void deleteFileHandleIfUnused(allocPointer_t fileHandle);
// Closes the given file, decrementing the open depth of the file handle. If the open depth reaches zero, the file handle is deleted.
// "fileHandle" is a pointer to a fileHandle_t.
void closeFile(allocPointer_t fileHandle);
void readFileRange(
    void *destination,
    // Pointer to a fileHandle_t.
    allocPointer_t fileHandle,
    // Offset of first byte to read.
    storageOffset_t pos,
    storageOffset_t amount
);
// Retrieves a list of all file names in the system volume. The output stores an array of pointers to dynamic allocations.
allocPointer_t getAllFileNames();
// Opens a file with the given name. Uses openFile for underlying logic.
// "stringAlloc" is a pointer to a dynamicAlloc_t containing the name of the file to open.
// Returns a pointer to a fileHandle_t.
allocPointer_t openFileByStringAlloc(allocPointer_t stringAlloc);

// Retrieves an index in the function table of the given running application.
// "runningApp" is a pointer to a runningApp_t.
int32_t findFuncById(allocPointer_t runningApp, int32_t funcId);
// Retrieves the running application which implements the caller of the currently active function invocation.
// Returns a pointer to a runningApp_t.
allocPointer_t getCurrentCaller();
// Creates an argument frame to be passed to the next function invocation.
// "size" is the size of data region in the new argument frame.
// Returns a pointer to a argFrame_t.
allocPointer_t createNextArgFrame(heapMemOffset_t size);
// "localFrame" is a pointer to a localFrame_t.
void cleanUpNextArgFrameHelper(allocPointer_t localFrame);
// Finds the lowest local frame in the given thread whose function is implemented by runningApp.
// "thread" is a pointer to a thread_t.
// "runningApp" is a pointer to a runningApp_t.
// Returns a pointer to a localFrame_t
allocPointer_t getBottomLocalFrame(allocPointer_t thread, allocPointer_t runningApp);

// Launches a running application from the given file handle.
// "fileHandle" is a pointer to a fileHandle_t.
void launchApp(allocPointer_t fileHandle);
// Performs the next possible kill action in the given running app.
// "runningApp" is a pointer to a runningApp_t.
void advanceKillAction(allocPointer_t runningApp);
// Requests the given application to clean up resources and quit.
// "runningApp" is a pointer to a runningApp_t.
void softKillApp(allocPointer_t runningApp);
// Terminates the given application without requesting it to clean up resources.
// "runningApp" is a pointer to a runningApp_t.
void hardKillApp(allocPointer_t runningApp, int8_t errorCode);

// Determines whether the given function is guarded.
// "implementer" is a pointer to a runningApp_t.
// "funcIndex" is the index of the function in "implementer".
int8_t funcIsGuarded(allocPointer_t implementer, int32_t funcIndex);
// Invokes a function in the given thread.
// "thread" is a pointer to a thread_t.
// "implementer" is a pointer to a runningApp_t.
// "funcIndex" is the index of the function in "implementer".
// "shouldCheckPerm" determines whether to check the permission of the current function implementer to invoke the given function.
void callFunc(
    allocPointer_t thread,
    allocPointer_t implementer,
    int32_t funcIndex,
    int8_t shouldCheckPerm
);
// Must be called after "callFunc" and "returnFromFunc" to update certain global variables.
void updateLocalFrameContext();
// Stops evaluation of the current function invocation, and returns control to the previous function invocation.
void returnFromFunc();

// Invokes the given function in a new thread, if the function exists.
// "runningApp" is a pointer to a runningApp_t.
// "funcId" is the ID of a function which runningApp implements.
// Returns whether the function exists in the running app.
int8_t createThread(allocPointer_t runningApp, int32_t funcId);
// Deletes the given thread.
// "thread" is a pointer to a thread_t.
void deleteThread(allocPointer_t thread);
// Causes an error handler to be invoked within the current thread.
void registerErrorInCurrentThread(int8_t error);

// Provides some time for the current thread to perform work.
void scheduleCurrentThread();
// Enters a blocking loop to run all WheatSystem applications.
void runAppSystem();

// Returns whether the given running application has admin permission.
// "runningApp" is a pointer to a runningApp_t.
int8_t runningAppHasAdminPerm(allocPointer_t runningApp);
// Returns whether the given running application has permission to read or modify the data region of the given dynamic allocation.
// "runningApp" is a pointer to a runningApp_t.
// "dynamicAlloc" is a pointer to a dynamicAlloc_t.
int8_t runningAppMayAccessAlloc(allocPointer_t runningApp, allocPointer_t dynamicAlloc);
// Returns whether the implementer of the current function has permission to read or modify the data region of the given dynamic allocation.
// "dynamicAlloc" is a pointer to a dynamicAlloc_t.
int8_t currentImplementerMayAccessAlloc(allocPointer_t dynamicAlloc);
// Returns whether the implementer of the current function has permission to read or modify the data region of the given file.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
int8_t currentImplementerMayAccessFile(allocPointer_t fileHandle);
// Changes whether the given file holds admin permission.
// "fileHandle" is an allocPointer_t to a fileHandle_t.
void setFileHasAdminPerm(allocPointer_t fileHandle, int8_t hasAdminPerm);

// Iterates over all allocations in the heap, calling "handle" for each allocation.
void iterateOverAllocs(void *context, allocIterationHandle_t handle);
void increaseRunningAppMemUsage(void *context, allocPointer_t alloc, allocPointer_t owner);
void registerRunningAppMemUsage(
    memUsageContext_t *context,
    allocPointer_t alloc,
    allocPointer_t owner
);
// Throws throttleErr in the current thread if any function invocation is implemented by the given running app.
// "runningApp" is a pointer to a runningApp_t.
// Returns whether throttleErr was thrown.
int8_t throttleAppInCurrentThread(allocPointer_t runningApp);
// Attempts to kill an application with the given action.
// "runningApp" is a pointer to a runningApp_t.
// Returns whether the action started successfully.
int8_t performKillAction(allocPointer_t runningApp, int8_t killAction);
// Controls how apps are throttled and killed.
void updateKillStates();

// Validates whether the given argument fits within its parent region. May set unhandledErrorCode to INDEX_ERR_CODE or LEN_ERR_CODE.
// "size" is the number of bytes to check from the beginning of the argument.
void validateArgBounds(instructionArg_t *arg, int32_t size);

// Reads a range of bytes in the given buffer argument. Does not verify bounds.
void readArgRange(
    int8_t *destination,
    instructionArg_t *arg,
    int32_t offset,
    uint8_t amount
);
// Writes a range of bytes in the given buffer argument. Does not verify bounds.
void writeArgRange(
    instructionArg_t *arg,
    int32_t offset,
    int8_t *source,
    uint8_t amount
);
// Helper functions for corresponding macros.
int32_t readArgIntHelper(instructionArg_t *arg);
void writeArgIntHelper(instructionArg_t *arg, int32_t value);
int32_t readArgConstantIntHelper(int8_t index);
void readArgRunningAppHelper(allocPointer_t *destination, int8_t index);

// Causes the bytecode interpreter to jump to the given position in the current function.
// "instructionOffset" is the offset from the beginning of the current function body.
void jumpToBytecodeInstruction(int32_t instructionOffset);
void parseInstructionArg(instructionArg_t *destination);
void evaluateWrtBuffInstruction();
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
// Implements the "kill" WheatSystem function.
void killTermApp();

// Resets global variables in the system.
void resetSystemState();


