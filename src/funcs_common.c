
#include "./headers.h"

allocPointer_t createAlloc(int8_t type, heapMemoryOffset_t size) {
    
    heapMemoryOffset_t sizeWithHeader = sizeof(allocHeader_t) + size;
    heapMemoryOffset_t startAddress = 0;
    allocPointer_t previousPointer = NULL_ALLOC_POINTER;
    allocPointer_t nextPointer = firstAlloc;
    
    // Find a gap which is large enough for the new allocation.
    while (nextPointer != NULL_ALLOC_POINTER) {
        heapMemoryOffset_t endAddress = convertPointerToAddress(nextPointer);
        if (endAddress - startAddress >= sizeWithHeader) {
            break;
        }
        startAddress = endAddress + getAllocSizeWithHeader(nextPointer);
        previousPointer = nextPointer;
        nextPointer = getAllocNext(nextPointer);
    }
    
    // Throw an error if there is not enough free memory.
    heapMemoryOffset_t endAddress = startAddress + sizeWithHeader;
    if (endAddress > HEAP_MEMORY_SIZE || endAddress < 0) {
        throw(CAPACITY_ERR_CODE, NULL_ALLOC_POINTER);
    }
    
    // Set up output allocation.
    allocPointer_t output = convertAddressToPointer(startAddress);
    setAllocMember(output, type, type);
    setAllocMember(output, size, size);
    setAllocMember(output, next, nextPointer);
    heapMemorySizeLeft -= sizeWithHeader;
    
    // Update previous allocation or firstAlloc.
    if (previousPointer == NULL_ALLOC_POINTER) {
        firstAlloc = output;
    } else {
        setAllocMember(previousPointer, next, output);
    }
    
    return output;
}

int8_t deleteAlloc(allocPointer_t pointer) {
    
    allocPointer_t previousPointer = NULL_ALLOC_POINTER;
    allocPointer_t nextPointer = firstAlloc;
    
    // Find previous and next allocations.
    while (true) {
        if (nextPointer == NULL_ALLOC_POINTER) {
            return false;
        }
        allocPointer_t tempPointer = nextPointer;
        nextPointer = getAllocNext(nextPointer);
        if (tempPointer == pointer) {
            break;
        }
        previousPointer = tempPointer;
    }
    
    
    // Update previous allocation or firstAlloc to
    // point to the next allocation.
    heapMemorySizeLeft += getAllocSizeWithHeader(pointer);
    if (previousPointer == NULL_ALLOC_POINTER) {
        firstAlloc = nextPointer;
    } else {
        setAllocMember(previousPointer, next, nextPointer);
    }
    
    return true;
}

void validateAllocPointer(allocPointer_t pointer) {
    if (pointer == NULL_ALLOC_POINTER) {
        throw(NULL_ERR_CODE);
    }
    allocPointer_t alloc = firstAlloc;
    while (alloc != NULL_ALLOC_POINTER) {
        if (alloc == pointer) {
            return;
        }
        alloc = getAllocNext(alloc);
    }
    throw(PTR_ERR_CODE);
}

allocPointer_t createDynamicAlloc(
    heapMemoryOffset_t size,
    int8_t attributes,
    allocPointer_t creator
) {
    allocPointer_t output = createAlloc(
        DYNAMIC_ALLOC_TYPE,
        sizeof(dynamicAllocHeader_t) + size
    );
    checkUnhandledError(NULL_ALLOC_POINTER);
    setDynamicAllocMember(output, attributes, attributes);
    setDynamicAllocMember(output, creator, creator);
    for (heapMemoryOffset_t index = 0; index < size; index++) {
        writeDynamicAlloc(output, index, int8_t, 0);
    }
    return output;
}

allocPointer_t createStringAllocFromFixedArrayHelper(
    const int8_t *fixedArray,
    heapMemoryOffset_t size
) {
    allocPointer_t output = createDynamicAlloc(size, GUARDED_ALLOC_ATTR, NULL_ALLOC_POINTER);
    checkUnhandledError(NULL_ALLOC_POINTER);
    for (heapMemoryOffset_t index = 0; index < size; index++) {
        int8_t character = readFixedArrayElement(fixedArray, index);
        writeDynamicAlloc(output, index, int8_t, character);
    }
    return output;
}

void validateDynamicAlloc(allocPointer_t dynamicAlloc) {
    validateAllocPointer(dynamicAlloc);
    checkUnhandledError();
    if (getAllocType(dynamicAlloc) != DYNAMIC_ALLOC_TYPE) {
        throw(TYPE_ERR_CODE);
    }
}

int8_t memoryNameEqualsStorageName(
    heapMemoryOffset_t memoryNameAddress,
    uint8_t memoryNameSize,
    storageOffset_t storageNameAddress,
    uint8_t storageNameSize
) {
    if (memoryNameSize != storageNameSize) {
        return false;
    }
    for (uint8_t index = 0; index < memoryNameSize; index++) {
        int8_t character1 = readHeapMemory(memoryNameAddress + index, int8_t);
        int8_t character2 = readStorageSpace(storageNameAddress + index, int8_t);
        if (character1 != character2) {
            return false;
        }
    }
    return true;
}

void copyStorageNameToMemory(
    storageOffset_t storageNameAddress,
    heapMemoryOffset_t memoryNameAddress,
    uint8_t nameSize
) {
    for (uint8_t index = 0; index < nameSize; index++) {
        int8_t character = readStorageSpace(storageNameAddress + index, int8_t);
        writeHeapMemory(memoryNameAddress + index, int8_t, character);
    }
}

void createFile(
    allocPointer_t name,
    int8_t type,
    int8_t isGuarded,
    storageOffset_t contentSize
) {
    
    // Verify argument values.
    heapMemoryOffset_t nameAllocAddress = getDynamicAllocDataAddress(name);
    heapMemoryOffset_t nameSize = getDynamicAllocSize(name);
    if (nameSize > 127) {
        throw(DATA_ERR_CODE);
    }
    if (type < GENERIC_FILE_TYPE || type > SYSTEM_APP_FILE_TYPE) {
        throw(TYPE_ERR_CODE);
    }
    if (contentSize < 0) {
        throw(LEN_ERR_CODE);
    }
    int fileStorageSize = getFileStorageSizeHelper(nameSize, contentSize);
    
    // Find a storage space gap which is large enough,
    // and ensure that there is no duplicate name.
    storageOffset_t previousFileAddress = MISSING_FILE_ADDRESS;
    storageOffset_t nextFileAddress = getFirstFileAddress();
    storageOffset_t newFileAddress = MISSING_FILE_ADDRESS;
    while (true) {
        int8_t hasReachedEnd = (nextFileAddress == MISSING_FILE_ADDRESS);
        if (newFileAddress == MISSING_FILE_ADDRESS) {
            storageOffset_t startAddress;
            if (previousFileAddress == MISSING_FILE_ADDRESS) {
                startAddress = sizeof(storageSpaceHeader_t);
            } else {
                startAddress = previousFileAddress + getFileStorageSize(previousFileAddress);
            }
            storageOffset_t endAddress = hasReachedEnd ? STORAGE_SPACE_SIZE : nextFileAddress;
            if (endAddress - startAddress >= fileStorageSize) {
                newFileAddress = startAddress;
            }
        }
        if (hasReachedEnd) {
            break;
        }
        if (memoryNameEqualsStorageName(
            nameAllocAddress,
            nameSize,
            getFileNameAddress(nextFileAddress),
            getFileHeaderMember(nextFileAddress, nameSize)
        )) {
            throw(DATA_ERR_CODE);
        }
        previousFileAddress = nextFileAddress;
        nextFileAddress = getFileHeaderMember(previousFileAddress, next);
    }
    if (newFileAddress == MISSING_FILE_ADDRESS) {
        throw(CAPACITY_ERR_CODE);
    }
    
    // Update file linked list.
    if (previousFileAddress == MISSING_FILE_ADDRESS) {
        setStorageSpaceMember(firstFileAddress, newFileAddress);
    } else {
        setFileHeaderMember(previousFileAddress, next, newFileAddress);
    }
    
    // Set up the new file.
    int8_t attributes = type;
    if (isGuarded) {
        attributes |= GUARDED_FILE_ATTR;
    }
    setFileHeaderMember(newFileAddress, attributes, attributes);
    setFileHeaderMember(newFileAddress, nameSize, nameSize);
    setFileHeaderMember(newFileAddress, contentSize, contentSize);
    setFileHeaderMember(newFileAddress, next, nextFileAddress);
    storageOffset_t nameAddress = newFileAddress + sizeof(fileHeader_t);
    for (uint8_t offset = 0; offset < nameSize; offset++) {
        int8_t character = readDynamicAlloc(name, offset, int8_t);
        writeStorageSpace(nameAddress + offset, int8_t, character);
    }
    storageOffset_t contentAddress = nameAddress + nameSize;
    for (storageOffset_t offset = 0; offset < contentSize; offset++) {
        writeStorageSpace(contentAddress + offset, int8_t, 0);
    }
    flushStorageSpace();
}

void deleteFile(allocPointer_t fileHandle) {
    storageOffset_t fileAddress = getFileHandleMember(fileHandle, address);
    
    // Find previous and next files.
    storageOffset_t previousFileAddress = MISSING_FILE_ADDRESS;
    storageOffset_t nextFileAddress = getFirstFileAddress();
    while (true) {
        if (nextFileAddress == MISSING_FILE_ADDRESS) {
            // This should not happen if invariants hold true.
            return;
        }
        storageOffset_t tempAddress = nextFileAddress;
        nextFileAddress = getFileHeaderMember(tempAddress, next);
        if (tempAddress == fileAddress) {
            break;
        }
        previousFileAddress = tempAddress;
    }
    
    // Delete the file.
    if (previousFileAddress == MISSING_FILE_ADDRESS) {
        setStorageSpaceMember(firstFileAddress, nextFileAddress);
    } else {
        setFileHeaderMember(previousFileAddress, next, nextFileAddress);
    }
    flushStorageSpace();
    deleteFileHandle(fileHandle);
}

storageOffset_t getFileAddressByName(
    heapMemoryOffset_t nameAddress,
    heapMemoryOffset_t nameSize
) {
    storageOffset_t fileAddress = getFirstFileAddress();
    while (fileAddress != MISSING_FILE_ADDRESS) {
        if (memoryNameEqualsStorageName(
            nameAddress,
            nameSize,
            getFileNameAddress(fileAddress),
            getFileHeaderMember(fileAddress, nameSize)
        )) {
            return fileAddress;
        }
        fileAddress = getFileHeaderMember(fileAddress, next);
    }
    return MISSING_FILE_ADDRESS;
}

storageOffset_t getFileStorageSize(storageOffset_t fileAddress) {
    int8_t nameSize = getFileHeaderMember(fileAddress, nameSize);
    storageOffset_t contentSize = getFileHeaderMember(fileAddress, contentSize);
    return getFileStorageSizeHelper(nameSize, contentSize);
}

allocPointer_t openFile(heapMemoryOffset_t nameAddress, heapMemoryOffset_t nameSize) {
    
    // Return matching file handle if it already exists.
    allocPointer_t nextPointer = firstAlloc;
    while (nextPointer != NULL_ALLOC_POINTER) {
        allocPointer_t pointer = nextPointer;
        nextPointer = getAllocNext(pointer);
        if (!allocIsFileHandle(pointer)) {
            continue;
        }
        if (!memoryNameEqualsStorageName(
            nameAddress,
            nameSize,
            getFileNameAddress(getFileHandleMember(pointer, address)),
            getFileHandleMember(pointer, nameSize)
        )) {
            continue;
        }
        int8_t depth = getFileHandleMember(pointer, openDepth);
        setFileHandleMember(pointer, openDepth, depth + 1);
        return pointer;
    }
    
    // Try to find file in storage.
    storageOffset_t fileAddress = getFileAddressByName(nameAddress, nameSize);
    if (fileAddress == MISSING_FILE_ADDRESS) {
        return NULL_ALLOC_POINTER;
    }
    
    // Read file header.
    uint8_t fileAttributes = getFileHeaderMember(fileAddress, attributes);
    storageOffset_t contentSize = getFileHeaderMember(fileAddress, contentSize);
    
    // Create file handle.
    allocPointer_t output = createDynamicAlloc(
        sizeof(fileHandle_t),
        GUARDED_ALLOC_ATTR | SENTRY_ALLOC_ATTR,
        NULL_ALLOC_POINTER
    );
    checkUnhandledError(NULL_ALLOC_POINTER);
    setFileHandleMember(output, address, fileAddress);
    setFileHandleMember(output, attributes, fileAttributes);
    setFileHandleMember(output, nameSize, nameSize);
    setFileHandleMember(output, contentSize, contentSize);
    setFileHandleMember(output, runningApp, NULL_ALLOC_POINTER);
    setFileHandleMember(output, initErr, NONE_ERR_CODE);
    setFileHandleMember(output, openDepth, 1);
    return output;
}

void deleteFileHandle(allocPointer_t fileHandle) {
    allocPointer_t runningApp = getFileHandleRunningApp(fileHandle);
    if (runningApp != NULL_ALLOC_POINTER) {
        hardKillApp(runningApp, MISSING_ERR_CODE);
    }
    deleteAlloc(fileHandle);
}

void deleteFileHandleIfUnused(allocPointer_t fileHandle) {
    int8_t openDepth = getFileHandleMember(fileHandle, openDepth);
    if (openDepth > 0) {
        return;
    }
    allocPointer_t runningApp = getFileHandleRunningApp(fileHandle);
    if (runningApp == NULL_ALLOC_POINTER) {
        deleteFileHandle(fileHandle);
    }
}

void closeFile(allocPointer_t fileHandle) {
    flushStorageSpace();
    int8_t openDepth = getFileHandleMember(fileHandle, openDepth);
    setFileHandleMember(fileHandle, openDepth, openDepth - 1);
    deleteFileHandleIfUnused(fileHandle);
}

void readFileRange(
    void *destination,
    allocPointer_t fileHandle,
    storageOffset_t pos,
    storageOffset_t amount
) {
    storageOffset_t address = getFileHandleDataAddress(fileHandle) + pos;
    readStorageSpaceRange(destination, address, amount);
}

allocPointer_t getAllFileNames() {
    
    // First find the number of files in the volume.
    int32_t fileCount = 0;
    storageOffset_t fileAddress = getFirstFileAddress();
    while (fileAddress != MISSING_FILE_ADDRESS) {
        fileCount += 1;
        fileAddress = getFileHeaderMember(fileAddress, next);
    }
    allocPointer_t output = createDynamicAlloc(
        fileCount * 4,
        GUARDED_ALLOC_ATTR,
        currentImplementer
    );
    checkUnhandledError(NULL_ALLOC_POINTER);
    
    // Then create an allocation for each file name.
    fileAddress = getFirstFileAddress();
    int32_t index = 0;
    while (index < fileCount) {
        uint8_t nameSize = getFileHeaderMember(fileAddress, nameSize);
        allocPointer_t nameAlloc = createDynamicAlloc(
            nameSize,
            GUARDED_ALLOC_ATTR,
            currentImplementer
        );
        if (unhandledErrorCode != NONE_ERR_CODE) {
            while (index > 0) {
                index -= 1;
                nameAlloc = readDynamicAlloc(output, index * 4, int32_t);
                deleteAlloc(nameAlloc);
            }
            deleteAlloc(output);
            return NULL_ALLOC_POINTER;
        }
        copyStorageNameToMemory(
            getFileNameAddress(fileAddress),
            getDynamicAllocDataAddress(nameAlloc),
            nameSize
        );
        writeDynamicAlloc(output, index * 4, int32_t, nameAlloc);
        fileAddress = getFileHeaderMember(fileAddress, next);
        index += 1;
    }
    return output;
}

allocPointer_t openFileByStringAlloc(allocPointer_t stringAlloc) {
    heapMemoryOffset_t address = getDynamicAllocDataAddress(stringAlloc);
    heapMemoryOffset_t size = getDynamicAllocSize(stringAlloc);
    return openFile(address, (uint8_t)size);
}

int8_t allocIsFileHandle(allocPointer_t pointer) {
    return (getAllocType(pointer) == DYNAMIC_ALLOC_TYPE
        && getDynamicAllocMember(pointer, creator) == NULL_ALLOC_POINTER
        && (getDynamicAllocMember(pointer, attributes) & SENTRY_ALLOC_ATTR));
}

void validateFileHandle(allocPointer_t fileHandle) {
    validateAllocPointer(fileHandle);
    checkUnhandledError();
    if (!allocIsFileHandle(fileHandle)) {
        throw(TYPE_ERR_CODE);
    }
}

int32_t findFunctionById(allocPointer_t runningApp, int32_t functionId) {
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        int32_t functionTableLength = getBytecodeGlobalFrameMember(
            runningApp,
            functionTableLength
        );
        for (int32_t index = 0; index < functionTableLength; index++) {
            int32_t tempId = getBytecodeFunctionMember(fileHandle, index, functionId);
            if (tempId == functionId) {
                return index;
            }
        }
    } else if (fileType == SYSTEM_APP_FILE_TYPE) {
        int8_t systemAppId = getSystemGlobalFrameMember(runningApp, id);
        const systemAppFunction_t *functionList = getSystemAppMember(
            systemAppId,
            functionList
        );
        int8_t functionAmount = getSystemAppMember(systemAppId, functionAmount);
        for (int8_t index = 0; index < functionAmount; index++) {
            int8_t tempId = getSystemAppFunctionListMember(functionList, index, id);
            if (tempId == functionId) {
                return index;
            }
        }
    }
    return -1;
}

allocPointer_t getCurrentCaller() {
    allocPointer_t previousLocalFrame = getLocalFrameMember(
        currentLocalFrame,
        previousLocalFrame
    );
    if (previousLocalFrame == NULL_ALLOC_POINTER) {
        return NULL_ALLOC_POINTER;
    }
    return getLocalFrameMember(previousLocalFrame, implementer);
}

allocPointer_t createNextArgFrame(heapMemoryOffset_t size) {
    cleanUpNextArgFrame();
    allocPointer_t output = createAlloc(
        ARG_FRAME_ALLOC_TYPE,
        sizeof(argFrameHeader_t) + size
    );
    checkUnhandledError(NULL_ALLOC_POINTER);
    setArgFrameMember(output, thread, currentThread);
    for (heapMemoryOffset_t index = 0; index < size; index++) {
        writeArgFrame(output, index, int8_t, 0);
    }
    setLocalFrameMember(currentLocalFrame, nextArgFrame, output);
    return output;
}

void cleanUpNextArgFrameHelper(allocPointer_t localFrame) {
    allocPointer_t nextArgFrame = getLocalFrameMember(localFrame, nextArgFrame);
    if (nextArgFrame != NULL_ALLOC_POINTER) {
        deleteAlloc(nextArgFrame);
    }
}

allocPointer_t getBottomLocalFrame(allocPointer_t thread, allocPointer_t runningApp) {
    allocPointer_t output = NULL_ALLOC_POINTER;
    allocPointer_t localFrame = getThreadMember(thread, localFrame);
    while (localFrame != NULL_ALLOC_POINTER) {
        allocPointer_t implementer = getLocalFrameMember(localFrame, implementer);
        if (implementer == runningApp) {
            output = localFrame;
        }
        localFrame = getLocalFrameMember(localFrame, previousLocalFrame);
    }
    return output;
}

void launchApp(allocPointer_t fileHandle) {
    
    // Do not launch app again if it is already running.
    allocPointer_t runningApp = getFileHandleRunningApp(fileHandle);
    if (runningApp != NULL_ALLOC_POINTER) {
        return;
    }
    int8_t fileType = getFileHandleType(fileHandle);
    storageOffset_t fileSize = getFileHandleMember(fileHandle, contentSize);
    int32_t functionTableLength;
    int32_t appDataFilePos;
    int8_t systemAppId;
    
    // Determine global frame size.
    heapMemoryOffset_t globalFrameSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        
        // Validate bytecode app file.
        if (fileSize < sizeof(bytecodeAppHeader_t)) {
            throw(DATA_ERR_CODE);
        }
        functionTableLength = getBytecodeAppMember(fileHandle, functionTableLength);
        appDataFilePos = getBytecodeAppMember(fileHandle, appDataFilePos);
        int32_t expectedFilePos = sizeof(bytecodeAppHeader_t) + functionTableLength * sizeof(bytecodeFunction_t);
        if (expectedFilePos > fileSize) {
            throw(DATA_ERR_CODE);
        }
        for (int32_t index = 0; index < functionTableLength; index++) {
            int32_t instructionBodyFilePos = getBytecodeFunctionMember(
                fileHandle,
                index,
                instructionBodyFilePos
            );
            int32_t instructionBodySize = getBytecodeFunctionMember(
                fileHandle,
                index,
                instructionBodySize
            );
            if (instructionBodyFilePos != expectedFilePos) {
                throw(DATA_ERR_CODE);
            }
            expectedFilePos += instructionBodySize;
        }
        if (expectedFilePos != appDataFilePos || appDataFilePos > fileSize) {
            throw(DATA_ERR_CODE);
        }
        
        globalFrameSize = sizeof(bytecodeGlobalFrameHeader_t) + (heapMemoryOffset_t)getBytecodeAppMember(fileHandle, globalFrameSize);
    } else if (fileType == SYSTEM_APP_FILE_TYPE) {
        
        // Validate system app file.
        if (fileSize != 1) {
            throw(DATA_ERR_CODE);
        }
        
        systemAppId = readFile(fileHandle, 0, int8_t);
        globalFrameSize = sizeof(systemGlobalFrameHeader_t) + getSystemAppMember(systemAppId, globalFrameSize);
    } else {
        throw(TYPE_ERR_CODE);
    }
    
    // Create allocation for the running app.
    runningApp = createAlloc(
        RUNNING_APP_ALLOC_TYPE,
        sizeof(runningAppHeader_t) + globalFrameSize
    );
    checkUnhandledError();
    setRunningAppMember(runningApp, fileHandle, fileHandle);
    setRunningAppMember(runningApp, shouldSkipWait, false);
    setRunningAppMember(runningApp, killAction, NONE_KILL_ACTION);
    setRunningAppMember(runningApp, previous, NULL_ALLOC_POINTER);
    setRunningAppMember(runningApp, next, firstRunningApp);
    if (firstRunningApp != NULL_ALLOC_POINTER) {
        setRunningAppMember(firstRunningApp, previous, runningApp);
    }
    firstRunningApp = runningApp;
    setFileHandleMember(fileHandle, runningApp, runningApp);
    setFileHandleMember(fileHandle, initErr, NONE_ERR_CODE);
    
    // Initialize global frame.
    for (heapMemoryOffset_t index = 0; index < globalFrameSize; index++) {
        writeGlobalFrame(runningApp, index, int8_t, 0);
    }
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        setBytecodeGlobalFrameMember(runningApp, functionTableLength, functionTableLength);
        setBytecodeGlobalFrameMember(runningApp, appDataFilePos, appDataFilePos);
        setBytecodeGlobalFrameMember(runningApp, appDataSize, fileSize - appDataFilePos);
    } else {
        setSystemGlobalFrameMember(runningApp, id, systemAppId);
    }
    
    // Call init function if available.
    createThread(runningApp, INIT_FUNC_ID);
}

void advanceKillAction(allocPointer_t runningApp) {
    int8_t killAction = getRunningAppMember(runningApp, killAction) + 1;
    while (killAction <= HARD_KILL_ACTION) {
        int8_t hasStarted = performKillAction(runningApp, killAction);
        if (hasStarted) {
            break;
        }
        killAction += 1;
    }
}

void softKillApp(allocPointer_t runningApp) {
    int8_t killAction = getRunningAppMember(runningApp, killAction);
    if (killAction == NONE_KILL_ACTION) {
        advanceKillAction(runningApp);
    }
}

void hardKillApp(allocPointer_t runningApp, int8_t errorCode) {
    
    // Delete all threads of the running app.
    allocPointer_t thread = firstThread;
    while (thread != NULL_ALLOC_POINTER) {
        allocPointer_t tempThread = thread;
        thread = getThreadMember(tempThread, next);
        if (getThreadMember(tempThread, runningApp) != runningApp) {
            continue;
        }
        // Delete local frames and argument frames.
        allocPointer_t localFrame = getThreadMember(tempThread, localFrame);
        while (localFrame != NULL_ALLOC_POINTER) {
            cleanUpNextArgFrameHelper(localFrame);
            allocPointer_t previousLocalFrame = getLocalFrameMember(
                localFrame,
                previousLocalFrame
            );
            deleteAlloc(localFrame);
            localFrame = previousLocalFrame;
        }
        deleteThread(tempThread);
    }
    
    // Throw errors in threads which belong to other apps.
    allocPointer_t lastThread = currentThread;
    thread = firstThread;
    while (thread != NULL_ALLOC_POINTER) {
        allocPointer_t tempThread = thread;
        thread = getThreadMember(tempThread, next);
        allocPointer_t bottomFrame = getBottomLocalFrame(tempThread, runningApp);
        if (bottomFrame == NULL_ALLOC_POINTER) {
            continue;
        }
        setCurrentThread(tempThread);
        while (currentLocalFrame != NULL_ALLOC_POINTER) {
            allocPointer_t localFrame = currentLocalFrame;
            returnFromFunction();
            if (localFrame == bottomFrame) {
                break;
            }
        }
        registerErrorInCurrentThread(STATE_ERR_CODE);
    }
    setCurrentThread(lastThread);
    
    // Delete dynamic allocations.
    allocPointer_t alloc = firstAlloc;
    while (alloc != NULL_ALLOC_POINTER) {
        allocPointer_t nextAlloc = getAllocNext(alloc);
        int8_t type = getAllocType(alloc);
        if (type == DYNAMIC_ALLOC_TYPE) {
            allocPointer_t creator = getDynamicAllocMember(alloc, creator);
            if (creator == runningApp) {
                deleteAlloc(alloc);
            }
        }
        alloc = nextAlloc;
    }
    
    // Update file handle and delete running app.
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    setFileHandleMember(fileHandle, runningApp, NULL_ALLOC_POINTER);
    deleteFileHandleIfUnused(fileHandle);
    allocPointer_t previousRunningApp = getRunningAppMember(runningApp, previous);
    allocPointer_t nextRunningApp = getRunningAppMember(runningApp, next);
    if (previousRunningApp == NULL_ALLOC_POINTER) {
        firstRunningApp = nextRunningApp;
    } else {
        setRunningAppMember(previousRunningApp, next, nextRunningApp);
    }
    if (nextRunningApp != NULL_ALLOC_POINTER) {
        setRunningAppMember(nextRunningApp, previous, previousRunningApp);
    }
    deleteAlloc(runningApp);
}

int8_t functionIsGuarded(allocPointer_t implementer, int32_t functionIndex) {
    allocPointer_t fileHandle = getRunningAppMember(implementer, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    const systemAppFunction_t *systemAppFunctionList;
    validateFunctionIndex(implementer, fileType, functionIndex, systemAppFunctionList, false);
    int8_t output;
    functionIsGuardedHelper(
        output,
        fileHandle,
        fileType,
        functionIndex,
        systemAppFunctionList
    );
    return output;
}

void callFunction(
    allocPointer_t thread,
    allocPointer_t implementer,
    int32_t functionIndex,
    int8_t shouldCheckPerm
) {
    
    allocPointer_t fileHandle = getRunningAppMember(implementer, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    allocPointer_t previousLocalFrame = getThreadMember(thread, localFrame);
    const systemAppFunction_t *systemAppFunctionList;
    
    // Validate function index.
    validateFunctionIndex(implementer, fileType, functionIndex, systemAppFunctionList);
    
    // Validate current implementer permission.
    if (shouldCheckPerm && currentImplementer != implementer
            && !currentImplementerHasAdminPerm()) {
        int8_t isGuarded;
        functionIsGuardedHelper(
            isGuarded,
            fileHandle,
            fileType,
            functionIndex,
            systemAppFunctionList
        );
        if (isGuarded) {
            throw(PERM_ERR_CODE);
        }
    }
    
    // Validate arg frame size.
    int32_t argFrameSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        argFrameSize = getBytecodeFunctionMember(fileHandle, functionIndex, argFrameSize);
    } else {
        argFrameSize = getSystemAppFunctionListMember(
            systemAppFunctionList,
            functionIndex,
            argFrameSize
        );
    }
    if (argFrameSize > 0) {
        allocPointer_t previousArgFrame;
        if (previousLocalFrame == NULL_ALLOC_POINTER) {
            previousArgFrame = NULL_ALLOC_POINTER;
        } else {
            previousArgFrame = getLocalFrameMember(previousLocalFrame, nextArgFrame);
        }
        if (previousArgFrame == NULL_ALLOC_POINTER
                || getArgFrameSize(previousArgFrame) != argFrameSize) {
            throw(ARG_FRAME_ERR_CODE);
        }
    }
    
    // Determine local frame size.
    heapMemoryOffset_t localVarsStartIndex;
    heapMemoryOffset_t localVarsSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        localVarsStartIndex = sizeof(bytecodeLocalFrameHeader_t);
        localVarsSize = (heapMemoryOffset_t)getBytecodeFunctionMember(fileHandle, functionIndex, localFrameSize);
    } else {
        localVarsStartIndex = 0;
        localVarsSize = getSystemAppFunctionListMember(
            systemAppFunctionList,
            functionIndex,
            localFrameSize
        );
    }
    int8_t shouldThrottle;
    if (previousLocalFrame == NULL_ALLOC_POINTER) {
        shouldThrottle = false;
    } else {
        shouldThrottle = getLocalFrameMember(previousLocalFrame, shouldThrottle);
    }
    
    // Create allocation for the local frame.
    allocPointer_t localFrame = createAlloc(
        LOCAL_FRAME_ALLOC_TYPE,
        sizeof(localFrameHeader_t) + localVarsStartIndex + localVarsSize
    );
    checkUnhandledError();
    setLocalFrameMember(localFrame, thread, thread);
    setLocalFrameMember(localFrame, implementer, implementer);
    setLocalFrameMember(localFrame, functionIndex, functionIndex);
    setLocalFrameMember(localFrame, previousLocalFrame, previousLocalFrame);
    setLocalFrameMember(localFrame, nextArgFrame, NULL_ALLOC_POINTER);
    setLocalFrameMember(localFrame, lastErrorCode, NONE_ERR_CODE);
    setLocalFrameMember(localFrame, shouldThrottle, shouldThrottle);
    
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        // Initialize members specific to bytecode functions.
        int32_t instructionBodyFilePos = getBytecodeFunctionMember(
            fileHandle,
            functionIndex,
            instructionBodyFilePos
        );
        int32_t instructionBodySize = getBytecodeFunctionMember(
            fileHandle,
            functionIndex,
            instructionBodySize
        );
        setBytecodeLocalFrameMember(
            localFrame,
            instructionBodyStartFilePos,
            instructionBodyFilePos
        );
        setBytecodeLocalFrameMember(
            localFrame,
            instructionBodyEndFilePos,
            instructionBodyFilePos + instructionBodySize
        );
        setBytecodeLocalFrameMember(localFrame, instructionFilePos, instructionBodyFilePos);
        setBytecodeLocalFrameMember(localFrame, errorHandler, -1);
    }
    // Clear local variables data.
    for (heapMemoryOffset_t offset = 0; offset < localVarsSize; offset++) {
        writeLocalFrame(localFrame, localVarsStartIndex + offset, int8_t, 0);
    }
    
    // Update thread local frame.
    setThreadMember(thread, localFrame, localFrame);
}

// Does not update currentThread.
void setCurrentLocalFrame(allocPointer_t localFrame) {
    currentLocalFrame = localFrame;
    if (currentLocalFrame == NULL_ALLOC_POINTER) {
        currentImplementer = NULL_ALLOC_POINTER;
        currentImplementerFileHandle = NULL_ALLOC_POINTER;
        currentImplementerFileType = NONE_FILE_TYPE;
    } else {
        currentImplementer = getLocalFrameMember(currentLocalFrame, implementer);
        currentImplementerFileHandle = getRunningAppMember(currentImplementer, fileHandle);
        currentImplementerFileType = getFileHandleType(currentImplementerFileHandle);
    }
}

void returnFromFunction() {
    cleanUpNextArgFrame();
    allocPointer_t previousLocalFrame = getLocalFrameMember(
        currentLocalFrame,
        previousLocalFrame
    );
    deleteAlloc(currentLocalFrame);
    setThreadMember(currentThread, localFrame, previousLocalFrame);
    setCurrentLocalFrame(previousLocalFrame);
}

int8_t createThread(allocPointer_t runningApp, int32_t functionId) {
    int32_t functionIndex = findFunctionById(runningApp, functionId);
    if (functionIndex < 0) {
        return false;
    }
    allocPointer_t thread = createAlloc(THREAD_ALLOC_TYPE, sizeof(thread_t));
    checkUnhandledError(true);
    setThreadMember(thread, runningApp, runningApp);
    setThreadMember(thread, functionId, functionId);
    setThreadMember(thread, localFrame, NULL_ALLOC_POINTER);
    setThreadMember(thread, isWaiting, false);
    setThreadMember(thread, previous, NULL_ALLOC_POINTER);
    setThreadMember(thread, next, firstThread);
    if (firstThread != NULL_ALLOC_POINTER) {
        setThreadMember(firstThread, previous, thread);
    }
    firstThread = thread;
    callFunction(thread, runningApp, functionIndex, false);
    if (nextThread == NULL_ALLOC_POINTER) {
        nextThread = thread;
    }
    return true;
}

void deleteThread(allocPointer_t thread) {
    allocPointer_t tempPreviousThread = getThreadMember(thread, previous);
    allocPointer_t tempNextThread = getThreadMember(thread, next);
    if (tempPreviousThread == NULL_ALLOC_POINTER) {
        firstThread = tempNextThread;
    } else {
        setThreadMember(tempPreviousThread, next, tempNextThread);
    }
    if (tempNextThread != NULL_ALLOC_POINTER) {
        setThreadMember(tempNextThread, previous, tempPreviousThread);
    }
    if (thread == nextThread) {
        advanceNextThread(nextThread);
        // If we cannot advance the next thread, there
        // are no more threads to schedule.
        if (thread == nextThread) {
            nextThread = NULL_ALLOC_POINTER;
        }
    }
    deleteAlloc(thread);
}

void registerErrorInCurrentThread(int8_t error) {
    while (true) {
        if (error == THROTTLE_ERR_CODE) {
            int8_t shouldThrottle = getLocalFrameMember(currentLocalFrame, shouldThrottle);
            if (!shouldThrottle) {
                error = STATE_ERR_CODE;
            }
        }
        int8_t shouldHandleError;
        if (currentImplementerFileType == BYTECODE_APP_FILE_TYPE) {
            int32_t instructionOffset = getBytecodeLocalFrameMember(
                currentLocalFrame,
                errorHandler
            );
            if (instructionOffset >= 0) {
                jumpToBytecodeInstruction(instructionOffset);
                shouldHandleError = true;
            } else {
                shouldHandleError = false;
            }
        } else {
            shouldHandleError = true;
        }
        if (shouldHandleError) {
            setLocalFrameMember(currentLocalFrame, lastErrorCode, error);
            setThreadMember(currentThread, isWaiting, false);
            break;
        }
        returnFromFunction();
        if (currentLocalFrame == NULL_ALLOC_POINTER) {
            if (getThreadMember(currentThread, functionId) == INIT_FUNC_ID) {
                allocPointer_t runningApp = getThreadMember(currentThread, runningApp);
                allocPointer_t fileHandle = getRunningAppMember(
                    runningApp,
                    fileHandle
                );
                setFileHandleMember(fileHandle, initErr, error);
            }
            break;
        }
    }
}

void setCurrentThread(allocPointer_t thread) {
    currentThread = thread;
    allocPointer_t localFrame = getThreadMember(currentThread, localFrame);
    setCurrentLocalFrame(localFrame);
}

void advanceNextThread(allocPointer_t previousThread) {
    nextThread = getThreadMember(previousThread, next);
    if (nextThread == NULL_ALLOC_POINTER) {
        nextThread = firstThread;
    }
}

void scheduleCurrentThread() {
    if (currentImplementerFileType == BYTECODE_APP_FILE_TYPE) {
        evaluateBytecodeInstruction();
    } else {
        int8_t functionIndex = (int8_t)getLocalFrameMember(currentLocalFrame, functionIndex);
        void (*threadAction)() = getRunningSystemAppFunctionMember(
            currentImplementer,
            functionIndex,
            threadAction
        );
        threadAction();
    }
    if (unhandledErrorCode != NONE_ERR_CODE) {
        if (currentThread != NULL_ALLOC_POINTER) {
            // currentThread will be null if the app quits while running.
            registerErrorInCurrentThread(unhandledErrorCode);
        }
        unhandledErrorCode = NONE_ERR_CODE;
    }
}

void runAppSystem() {
    
    // Launch boot application.
    resetHaltFlag();
    allocPointer_t bootFileName = createStringAllocFromFixedArray(bootStringConstant);
    checkUnhandledError();
    allocPointer_t bootFileHandle = openFileByStringAlloc(bootFileName);
    checkUnhandledError();
    deleteAlloc(bootFileName);
    checkUnhandledError();
    if (bootFileHandle == NULL_ALLOC_POINTER) {
        return;
    }
    launchApp(bootFileHandle);
    checkUnhandledError();
    closeFile(bootFileHandle);
    nextThread = firstThread;
    
    // Enter loop scheduling app threads.
    while (nextThread != NULL_ALLOC_POINTER) {
        checkHaltFlag();
        killStatesDelay += 1;
        if (killStatesDelay >= KILL_STATES_PERIOD) {
            updateKillStates();
            killStatesDelay = 0;
        }
        allocPointer_t thread = nextThread;
        advanceNextThread(thread);
        int8_t isWaiting = getThreadMember(thread, isWaiting);
        if (!isWaiting) {
            setCurrentThread(thread);
            if (currentLocalFrame == NULL_ALLOC_POINTER) {
                deleteThread(currentThread);
            } else {
                scheduleCurrentThread();
            }
        }
    }
}

int8_t runningAppHasAdminPerm(allocPointer_t runningApp) {
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    int8_t attributes = getFileHandleMember(fileHandle, attributes);
    return getHasAdminPermFromFileAttributes(attributes);
}

int8_t currentImplementerHasAdminPerm() {
    int8_t attributes = getFileHandleMember(currentImplementerFileHandle, attributes);
    return getHasAdminPermFromFileAttributes(attributes);
}

int8_t runningAppMayAccessAlloc(allocPointer_t runningApp, allocPointer_t dynamicAlloc) {
    runningAppMayAccessAllocHelper(runningApp, dynamicAlloc);
    return runningAppHasAdminPerm(runningApp);
}

int8_t currentImplementerMayAccessAlloc(allocPointer_t dynamicAlloc) {
    runningAppMayAccessAllocHelper(currentImplementer, dynamicAlloc);
    return currentImplementerHasAdminPerm();
}

int8_t currentImplementerMayAccessFile(allocPointer_t fileHandle) {
    if (currentImplementerHasAdminPerm()) {
        return true;
    }
    int8_t attributes = getFileHandleMember(fileHandle, attributes);
    return !getIsGuardedFromFileAttributes(attributes);
}

void setFileHasAdminPerm(allocPointer_t fileHandle, int8_t hasAdminPerm) {
    uint8_t attributes = getFileHandleMember(fileHandle, attributes);
    if (hasAdminPerm) {
        attributes |= ADMIN_FILE_ATTR;
    } else {
        attributes &= ~ADMIN_FILE_ATTR;
    }
    setFileHandleMember(fileHandle, attributes, attributes);
    storageOffset_t address = getFileHandleMember(fileHandle, address);
    setFileHeaderMember(address, attributes, attributes);
    flushStorageSpace();
}

int8_t throttleAppInCurrentThread(allocPointer_t runningApp) {
    allocPointer_t bottomFrame = getBottomLocalFrame(currentThread, runningApp);
    if (bottomFrame == NULL_ALLOC_POINTER) {
        return false;
    }
    allocPointer_t localFrame = currentLocalFrame;
    while (localFrame != NULL_ALLOC_POINTER) {
        setLocalFrameMember(localFrame, shouldThrottle, true);
        if (localFrame == bottomFrame) {
            break;
        }
        localFrame = getLocalFrameMember(localFrame, previousLocalFrame);
    }
    registerErrorInCurrentThread(THROTTLE_ERR_CODE);
    return true;
}

int8_t performKillAction(allocPointer_t runningApp, int8_t killAction) {
    int8_t hasStarted = false;
    if (killAction == WARN_KILL_ACTION) {
        hasStarted = createThread(runningApp, KILL_FUNC_ID);
        checkUnhandledError(false);
    } else if (killAction == THROTTLE_KILL_ACTION) {
        int16_t throttleCount = 0;
        allocPointer_t lastThread = currentThread;
        allocPointer_t thread = firstThread;
        while (thread != NULL_ALLOC_POINTER) {
            setCurrentThread(thread);
            thread = getThreadMember(currentThread, next);
            int8_t hasThrottled = throttleAppInCurrentThread(runningApp);
            if (hasThrottled) {
                throttleCount += 1;
            }
        }
        setCurrentThread(lastThread);
        hasStarted = (throttleCount > 0);
    } else if (killAction == HARD_KILL_ACTION) {
        hardKillApp(runningApp, THROTTLE_ERR_CODE);
        // hardKillApp deletes runningApp, so we don't want to update
        // members of runningApp after calling hardKillApp.
        return true;
    }
    if (hasStarted) {
        setRunningAppMember(runningApp, killAction, killAction);
        setRunningAppMember(runningApp, killActionDelay, 0);
    }
    return hasStarted;
}

allocPointer_t getAllocOwner(allocPointer_t pointer) {
    int8_t type = getAllocType(pointer);
    if (type == RUNNING_APP_ALLOC_TYPE) {
        return pointer;
    } else if (type == THREAD_ALLOC_TYPE) {
        return getThreadMember(pointer, runningApp);
    } else if (type == LOCAL_FRAME_ALLOC_TYPE) {
        allocPointer_t thread = getLocalFrameMember(pointer, thread);
        return getThreadMember(thread, runningApp);
    } else if (type == ARG_FRAME_ALLOC_TYPE) {
        allocPointer_t thread = getArgFrameMember(pointer, thread);
        return getThreadMember(thread, runningApp);
    } else if (type == DYNAMIC_ALLOC_TYPE) {
        return getDynamicAllocMember(pointer, creator);
    }
    return NULL_ALLOC_POINTER;
}

void updateKillStates() {
    
    // Advance states of apps which are currently being killed.
    int16_t killActionCount = 0;
    allocPointer_t runningApp;
    allocPointer_t nextRunningApp = firstRunningApp;
    while (nextRunningApp != NULL_ALLOC_POINTER) {
        runningApp = nextRunningApp;
        nextRunningApp = getRunningAppMember(runningApp, next);
        int8_t killAction = getRunningAppMember(runningApp, killAction);
        if (killAction == NONE_KILL_ACTION) {
            continue;
        }
        killActionCount += 1;
        int8_t delay = getRunningAppMember(runningApp, killActionDelay);
        delay += 1;
        if (delay > 5) {
            advanceKillAction(runningApp);
        } else {
            setRunningAppMember(runningApp, killActionDelay, delay);
        }
    }
    
    // If we are already killing an app, or we have enough
    // memory, don't kill more apps to free memory.
    if (killActionCount > 0 || heapMemorySizeLeft > KILL_PANIC_THRESHOLD) {
        return;
    }
    
    // Calculate memory usage for all running apps.
    runningApp = firstRunningApp;
    while (runningApp != NULL_ALLOC_POINTER) {
        setRunningAppMember(runningApp, memoryUsage, 0);
        runningApp = getRunningAppMember(runningApp, next);
    }
    allocPointer_t pointer = firstAlloc;
    while (pointer != NULL_ALLOC_POINTER) {
        allocPointer_t owner = getAllocOwner(pointer);
        if (owner != NULL_ALLOC_POINTER) {
            heapMemoryOffset_t memoryUsage = getRunningAppMember(owner, memoryUsage);
            memoryUsage += getAllocSizeWithHeader(pointer);
            setRunningAppMember(owner, memoryUsage, memoryUsage);
        }
        pointer = getAllocNext(pointer);
    }
    
    // Determine the best app to kill.
    allocPointer_t victimRunningApp = NULL_ALLOC_POINTER;
    allocPointer_t victimMemoryUsage = 0;
    allocPointer_t victimHasAdminPerm = true;
    runningApp = firstRunningApp;
    while (runningApp != NULL_ALLOC_POINTER) {
        heapMemoryOffset_t memoryUsage = getRunningAppMember(runningApp, memoryUsage);
        int8_t hasAdminPerm = runningAppHasAdminPerm(runningApp);
        if ((hasAdminPerm == victimHasAdminPerm && memoryUsage > victimMemoryUsage)
                || (!hasAdminPerm && victimHasAdminPerm)) {
            victimRunningApp = runningApp;
            victimMemoryUsage = memoryUsage;
            victimHasAdminPerm = hasAdminPerm;
        }
        runningApp = getRunningAppMember(runningApp, next);
    }
    
    // Start killing the app to free memory.
    if (victimRunningApp != NULL_ALLOC_POINTER) {
        softKillApp(victimRunningApp);
    }
}

void validateArgBounds(instructionArg_t *arg, int32_t size) {
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    int32_t index;
    int32_t regionSize;
    if (referenceType == CONSTANT_REF_TYPE) {
        return;
    } else if (referenceType == APP_DATA_REF_TYPE) {
        index = arg->appDataIndex;
        regionSize = getBytecodeGlobalFrameMember(currentImplementer, appDataSize);
    } else {
        index = arg->index;
        regionSize = arg->size;
    }
    if (index < 0 || index >= regionSize) {
        throw(INDEX_ERR_CODE);
    }
    if (size < 0 || index + size > regionSize) {
        throw(LEN_ERR_CODE);
    }
}

int32_t readArgIntHelper2(instructionArg_t *arg, int32_t offset, int8_t dataType) {
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    if (referenceType == CONSTANT_REF_TYPE) {
        return arg->constantValue;
    } else if (referenceType == APP_DATA_REF_TYPE) {
        int32_t filePos = getBytecodeGlobalFrameMember(
            currentImplementer,
            appDataFilePos
        ) + arg->appDataIndex + offset;
        if (dataType == SIGNED_INT_8_TYPE) {
            return readFile(currentImplementerFileHandle, filePos, int8_t);
        } else {
            return readFile(currentImplementerFileHandle, filePos, int32_t);
        }
    } else {
        heapMemoryOffset_t address = arg->startAddress + arg->index + offset;
        if (dataType == SIGNED_INT_8_TYPE) {
            return readHeapMemory(address, int8_t);
        } else {
            return readHeapMemory(address, int32_t);
        }
    }
}

void writeArgIntHelper2(
    instructionArg_t *arg,
    int32_t offset,
    int8_t dataType,
    int32_t value
) {
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    if (referenceType == HEAP_MEM_REF_TYPE) {
        heapMemoryOffset_t address = arg->startAddress + arg->index + offset;
        if (dataType == SIGNED_INT_8_TYPE) {
            writeHeapMemory(address, int8_t, (int8_t)value);
        } else {
            writeHeapMemory(address, int32_t, value);
        }
    } else {
        throw(TYPE_ERR_CODE);
    }
}

int32_t readArgIntHelper1(instructionArg_t *arg) {
    uint8_t prefix = arg->prefix;
    uint8_t dataType = getArgPrefixDataType(prefix);
    int8_t dataTypeSize = getArgDataTypeSize(dataType);
    validateArgBounds(arg, dataTypeSize);
    checkUnhandledError(0);
    return readArgIntHelper2(arg, 0, dataType);
}

void writeArgIntHelper1(instructionArg_t *arg, int32_t value) {
    uint8_t prefix = arg->prefix;
    uint8_t dataType = getArgPrefixDataType(prefix);
    int8_t dataTypeSize = getArgDataTypeSize(dataType);
    validateArgBounds(arg, dataTypeSize);
    checkUnhandledError();
    writeArgIntHelper2(arg, 0, dataType, value);
}

int32_t readArgConstantIntHelper(int8_t index) {
    instructionArg_t *arg = instructionArgArray + index;
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    if (referenceType != CONSTANT_REF_TYPE) {
        throw(TYPE_ERR_CODE, 0);
    }
    return arg->constantValue;
}

void readArgRunningAppHelper(allocPointer_t *destination, int8_t index) {
    allocPointer_t appHandle = readArgFileHandle(index);
    allocPointer_t runningApp = getFileHandleRunningApp(appHandle);
    if (runningApp == NULL_ALLOC_POINTER) {
        if (getFileHandleType(appHandle) == GENERIC_FILE_TYPE) {
            throw(TYPE_ERR_CODE);
        } else {
            throw(STATE_ERR_CODE);
        }
    }
    *destination = runningApp;
}

void jumpToBytecodeInstruction(int32_t instructionOffset) {
    int32_t instructionBodyFilePos = getBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionBodyStartFilePos
    );
    setBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionFilePos,
        instructionBodyFilePos + instructionOffset
    );
}

void parseInstructionArg(instructionArg_t *destination) {
    uint8_t argPrefix = readInstructionData(uint8_t);
    uint8_t referenceType = getArgPrefixReferenceType(argPrefix);
    uint8_t dataType = getArgPrefixDataType(argPrefix);
    if (dataType > SIGNED_INT_32_TYPE || referenceType > DYNAMIC_ALLOC_REF_TYPE) {
        throw(TYPE_ERR_CODE);
    }
    if (referenceType == CONSTANT_REF_TYPE) {
        destination->prefix = argPrefix;
        if (dataType == SIGNED_INT_8_TYPE) {
            destination->constantValue = readInstructionData(int8_t);
        } else {
            destination->constantValue = readInstructionData(int32_t);
        }
    } else {
        instructionArg_t arg1;
        parseInstructionArg(&arg1);
        checkUnhandledError();
        int32_t argValue1 = readArgIntHelper1(&arg1);
        checkUnhandledError();
        if (referenceType == APP_DATA_REF_TYPE) {
            destination->prefix = argPrefix;
            destination->appDataIndex = argValue1;
        } else {
            heapMemoryOffset_t startAddress;
            heapMemoryOffset_t size;
            heapMemoryOffset_t index;
            if (referenceType == DYNAMIC_ALLOC_REF_TYPE) {
                validateDynamicAlloc(argValue1);
                checkUnhandledError();
                if (!currentImplementerMayAccessAlloc(argValue1)) {
                    throw(PERM_ERR_CODE);
                }
                startAddress = getDynamicAllocDataAddress(argValue1);
                instructionArg_t arg2;
                parseInstructionArg(&arg2);
                checkUnhandledError();
                index = (heapMemoryOffset_t)readArgIntHelper1(&arg2);
                checkUnhandledError();
                size = getDynamicAllocSize(argValue1);
            } else {
                index = (heapMemoryOffset_t)argValue1;
                if (referenceType == GLOBAL_FRAME_REF_TYPE) {
                    startAddress = getBytecodeGlobalFrameDataAddress(currentImplementer);
                    size = getBytecodeGlobalFrameSize(currentImplementer);
                } else if (referenceType == LOCAL_FRAME_REF_TYPE) {
                    startAddress = getBytecodeLocalFrameDataAddress(currentLocalFrame);
                    size = getBytecodeLocalFrameSize(currentLocalFrame);
                } else {
                    allocPointer_t localFrame;
                    if (referenceType == PREV_ARG_FRAME_REF_TYPE) {
                        localFrame = getLocalFrameMember(
                            currentLocalFrame,
                            previousLocalFrame
                        );
                        if (localFrame == NULL_ALLOC_POINTER) {
                            throw(ARG_FRAME_ERR_CODE);
                        }
                    } else {
                        // referenceType must equal NEXT_ARG_FRAME_REF_TYPE.
                        localFrame = currentLocalFrame;
                    }
                    allocPointer_t argFrame = getLocalFrameMember(
                        localFrame,
                        nextArgFrame
                    );
                    if (argFrame == NULL_ALLOC_POINTER) {
                        throw(ARG_FRAME_ERR_CODE);
                    }
                    startAddress = getArgFrameDataAddress(argFrame);
                    size = getArgFrameSize(argFrame);
                }
            }
            destination->prefix = (HEAP_MEM_REF_TYPE << 4) | dataType;
            destination->startAddress = startAddress;
            destination->size = size;
            destination->index = index;
        }
    }
}

void evaluateWrtBuffInstruction() {
    instructionArg_t *destination = instructionArgArray;
    instructionArg_t *source = instructionArgArray + 1;
    int32_t size = readArgInt(2);
    validateArgBounds(destination, size);
    checkUnhandledError();
    validateArgBounds(source, size);
    checkUnhandledError();
    int8_t shouldCopyBackward;
    uint8_t destRefType = getArgPrefixReferenceType(destination->prefix);
    uint8_t sourceRefType = getArgPrefixReferenceType(source->prefix);
    if (destRefType == HEAP_MEM_REF_TYPE && sourceRefType == HEAP_MEM_REF_TYPE) {
        heapMemoryOffset_t destStartAddress = destination->startAddress + destination->index;
        heapMemoryOffset_t sourceStartAddress = source->startAddress + source->index;
        shouldCopyBackward = (destStartAddress > sourceStartAddress);
    } else {
        shouldCopyBackward = false;
    }
    int8_t direction;
    int32_t startOffset;
    int32_t endOffset;
    if (shouldCopyBackward) {
        direction = -1;
        startOffset = size - 1;
        endOffset = -1;
    } else {
        direction = 1;
        startOffset = 0;
        endOffset = size;
    }
    for (int32_t offset = startOffset; offset != endOffset; offset += direction) {
        int8_t value = readArgIntHelper2(source, offset, SIGNED_INT_8_TYPE);
        checkUnhandledError();
        writeArgIntHelper2(destination, offset, SIGNED_INT_8_TYPE, value);
        checkUnhandledError();
    }
}

void evaluateBytecodeInstruction() {
    instructionBodyStartFilePos = getBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionBodyStartFilePos
    );
    instructionBodyEndFilePos = getBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionBodyEndFilePos
    );
    currentInstructionFilePos = getBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionFilePos
    );
    if (currentInstructionFilePos == instructionBodyEndFilePos) {
        returnFromFunction();
        return;
    }
    uint8_t opcode = readInstructionData(uint8_t);
    uint8_t opcodeCategory = opcode >> 4;
    if (opcodeCategory >= getArrayLength(argumentAmountOffsetArray) - 1) {
        throw(NO_IMPL_ERR_CODE);
    }
    uint8_t opcodeOffset = opcode & 0x0F;
    int8_t offset1 = readFixedArrayElement(argumentAmountOffsetArray, opcodeCategory);
    int8_t offset2 = readFixedArrayElement(argumentAmountOffsetArray, opcodeCategory + 1);
    if (opcodeOffset >= offset2 - offset1) {
        throw(NO_IMPL_ERR_CODE);
    }
    int8_t argumentAmount = readFixedArrayElement(
        argumentAmountArray,
        offset1 + opcodeOffset
    );
    for (int8_t index = 0; index < argumentAmount; index++) {
        parseInstructionArg(instructionArgArray + index);
        checkUnhandledError();
    }
    setBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionFilePos,
        currentInstructionFilePos
    );
    handleTestInstruction();
    if (opcodeCategory == 0x0) {
        // Memory instructions.
        if (opcodeOffset == 0x0) {
            // wrt.
            int32_t value = readArgInt(1);
            writeArgInt(0, value);
        } else if (opcodeOffset == 0x1) {
            // wrtBuff.
            evaluateWrtBuffInstruction();
        } else if (opcodeOffset == 0x2) {
            // newArgFrame.
            heapMemoryOffset_t argFrameSize = (heapMemoryOffset_t)readArgInt(0);
            if (argFrameSize < 0) {
                throw(LEN_ERR_CODE);
            }
            createNextArgFrame(argFrameSize);
        } else if (opcodeOffset == 0x3) {
            // newAlloc.
            int8_t attributes = (int8_t)readArgInt(1) & ALLOC_ATTR_MASK;
            heapMemoryOffset_t size = (heapMemoryOffset_t)readArgInt(2);
            if (size < 0) {
                throw(LEN_ERR_CODE);
            }
            allocPointer_t alloc = createDynamicAlloc(
                size,
                attributes,
                currentImplementer
            );
            checkUnhandledError();
            writeArgInt(0, alloc);
        } else if (opcodeOffset == 0x4) {
            // delAlloc.
            allocPointer_t alloc = readArgDynamicAlloc(0);
            if (!currentImplementerMayAccessAlloc(alloc)) {
                throw(PERM_ERR_CODE);
            }
            deleteAlloc(alloc);
        } else if (opcodeOffset == 0x5) {
            // allocAttrs.
            allocPointer_t alloc = readArgDynamicAlloc(1);
            int8_t attributes = getDynamicAllocMember(alloc, attributes);
            writeArgInt(0, attributes);
        } else if (opcodeOffset == 0x6) {
            // allocSize.
            allocPointer_t alloc = readArgDynamicAlloc(1);
            heapMemoryOffset_t size = getDynamicAllocSize(alloc);
            writeArgInt(0, size);
        } else if (opcodeOffset == 0x7) {
            // allocCreator.
            allocPointer_t alloc = readArgDynamicAlloc(1);
            allocPointer_t creator = getDynamicAllocMember(alloc, creator);
            allocPointer_t fileHandle = getRunningAppMember(creator, fileHandle);
            writeArgInt(0, fileHandle);
        } else if (opcodeOffset == 0x8) {
            // setAllocAttrs.
            allocPointer_t alloc = readArgDynamicAlloc(0);
            if (!currentImplementerMayAccessAlloc(alloc)) {
                throw(PERM_ERR_CODE);
            }
            int8_t attributes = readArgInt(1) & ALLOC_ATTR_MASK;
            setDynamicAllocMember(alloc, attributes, attributes);
        }
    } else if (opcodeCategory == 0x1) {
        // Control flow instructions.
        if (opcodeOffset == 0x0) {
            // jmp.
            int32_t instructionOffset = readArgConstantInt(0);
            jumpToBytecodeInstruction(instructionOffset);
        } else if (opcodeOffset == 0x1) {
            // jmpZ.
            int32_t condition = readArgInt(1);
            if (condition == 0) {
                int32_t instructionOffset = readArgConstantInt(0);
                jumpToBytecodeInstruction(instructionOffset);
            }
        } else if (opcodeOffset == 0x2) {
            // jmpNZ.
            int32_t condition = readArgInt(1);
            if (condition != 0) {
                int32_t instructionOffset = readArgConstantInt(0);
                jumpToBytecodeInstruction(instructionOffset);
            }
        } else if (opcodeOffset == 0x3) {
            // wait.
            int8_t shouldSkipWait = getRunningAppMember(currentImplementer, shouldSkipWait);
            if (shouldSkipWait) {
                setRunningAppMember(currentImplementer, shouldSkipWait, false);
            } else {
                setThreadMember(currentThread, isWaiting, true);
            }
        } else if (opcodeOffset == 0x4) {
            // resume.
            int8_t hasResumed = false;
            allocPointer_t thread = firstThread;
            while (thread != NULL_ALLOC_POINTER) {
                allocPointer_t localFrame = getThreadMember(thread, localFrame);
                allocPointer_t implementer = getLocalFrameMember(localFrame, implementer);
                if (implementer == currentImplementer) {
                    int8_t isWaiting = getThreadMember(thread, isWaiting);
                    if (isWaiting) {
                        setThreadMember(thread, isWaiting, false);
                        hasResumed = true;
                    }
                }
                thread = getThreadMember(thread, next);
            }
            if (!hasResumed) {
                setRunningAppMember(currentImplementer, shouldSkipWait, true);
            }
        }
    } else if (opcodeCategory == 0x2) {
        // Error instructions.
        if (opcodeOffset == 0x0) {
            // setErrJmp.
            int32_t instructionOffset = readArgConstantInt(0);
            setBytecodeLocalFrameMember(currentLocalFrame, errorHandler, instructionOffset);
        } else if (opcodeOffset == 0x1) {
            // clrErrJmp.
            setBytecodeLocalFrameMember(currentLocalFrame, errorHandler, -1);
        } else if (opcodeOffset == 0x2) {
            // throw.
            int32_t code = readArgInt(0);
            if (code < -128 || code > 127) {
                throw(NUM_RANGE_ERR_CODE);
            }
            throw(code);
        } else if (opcodeOffset == 0x3) {
            // err.
            int8_t code = getLocalFrameMember(currentLocalFrame, lastErrorCode);
            writeArgInt(0, code);
        }
    } else if (opcodeCategory == 0x3) {
        // Function instructions.
        if (opcodeOffset == 0x0) {
            // findFunc.
            allocPointer_t runningApp = readArgRunningApp(1);
            int32_t functionId = readArgInt(2);
            int32_t functionIndex = findFunctionById(runningApp, functionId);
            writeArgInt(0, functionIndex);
        } else if (opcodeOffset == 0x1) {
            // call.
            int32_t functionIndex = readArgInt(0);
            callFunction(currentThread, currentImplementer, functionIndex, false);
        } else if (opcodeOffset == 0x2) {
            // callRemote.
            allocPointer_t implementer = readArgRunningApp(0);
            int32_t functionIndex = readArgInt(1);
            callFunction(currentThread, implementer, functionIndex, true);
        } else if (opcodeOffset == 0x3) {
            // ret.
            returnFromFunction();
        } else if (opcodeOffset == 0x4) {
            // caller.
            allocPointer_t caller = getCurrentCaller();
            allocPointer_t fileHandle;
            if (caller == NULL_ALLOC_POINTER) {
                fileHandle = NULL_ALLOC_POINTER;
            } else {
                fileHandle = getRunningAppMember(caller, fileHandle);
            }
            writeArgInt(0, fileHandle);
        } else if (opcodeOffset == 0x5) {
            // funcIsGuarded.
            allocPointer_t implementer = readArgRunningApp(1);
            int32_t functionIndex = readArgInt(2);
            int8_t isGuarded = functionIsGuarded(implementer, functionIndex);
            checkUnhandledError();
            writeArgInt(0, isGuarded);
        }
    } else if (opcodeCategory == 0x4) {
        // Bitwise instructions.
        uint32_t result = 0;
        if (opcodeOffset == 0x0) {
            // bNot.
            uint32_t operand = readArgInt(1);
            result = ~operand;
        } else {
            uint32_t operand1 = readArgInt(1);
            uint32_t operand2 = readArgInt(2);
            if (opcodeOffset == 0x1) {
                // bOr.
                result = (operand1 | operand2);
            } else if (opcodeOffset == 0x2) {
                // bAnd.
                result = (operand1 & operand2);
            } else if (opcodeOffset == 0x3) {
                // bXor.
                result = (operand1 ^ operand2);
            } else if (opcodeOffset == 0x4) {
                // bLeft.
                result = (operand1 << operand2);
            } else if (opcodeOffset == 0x5) {
                // bRight.
                result = (operand1 >> operand2);
            }
        }
        writeArgInt(0, result);
    } else if (opcodeCategory == 0x5) {
        // Comparison instructions.
        int32_t operand1 = readArgInt(1);
        int32_t operand2 = readArgInt(2);
        int32_t result = 0;
        if (opcodeOffset == 0x0) {
            // equ.
            result = (operand1 == operand2);
        } else if (opcodeOffset == 0x1) {
            // nEqu.
            result = (operand1 != operand2);
        } else if (opcodeOffset == 0x2) {
            // gre.
            result = (operand1 > operand2);
        } else if (opcodeOffset == 0x3) {
            // nGre.
            result = (operand1 <= operand2);
        }
        writeArgInt(0, result);
    } else if (opcodeCategory == 0x6) {
        // Arithmetic instructions.
        int32_t operand1 = readArgInt(1);
        int32_t operand2 = readArgInt(2);
        int32_t result = 0;
        if (opcodeOffset == 0x0) {
            // add.
            result = operand1 + operand2;
        } else if (opcodeOffset == 0x1) {
            // sub.
            result = operand1 - operand2;
        } else if (opcodeOffset == 0x2) {
            // mul.
            result = operand1 * operand2;
        } else if (opcodeOffset == 0x3) {
            // div.
            if (operand2 == 0) {
                throw(NUM_RANGE_ERR_CODE);
            }
            result = operand1 / operand2;
        } else if (opcodeOffset == 0x4) {
            // mod.
            if (operand2 == 0) {
                throw(NUM_RANGE_ERR_CODE);
            }
            result = operand1 % operand2;
        }
        writeArgInt(0, result);
    } else if (opcodeCategory == 0x7) {
        // Application instructions.
        if (opcodeOffset == 0x0) {
            // launch.
            allocPointer_t appHandle = readArgFileHandle(0);
            launchApp(appHandle);
        } else if (opcodeOffset == 0x1) {
            // thisApp.
            writeArgInt(0, currentImplementerFileHandle);
        } else if (opcodeOffset == 0x2) {
            // quitApp.
            hardKillApp(currentImplementer, NONE_ERR_CODE);
        } else if (opcodeOffset == 0x3) {
            // appIsRunning.
            allocPointer_t appHandle = readArgFileHandle(1);
            allocPointer_t runningApp = getFileHandleRunningApp(appHandle);
            writeArgInt(0, (runningApp != NULL_ALLOC_POINTER));
        } else if (opcodeOffset == 0x4) {
            // appInitErr.
            allocPointer_t appHandle = readArgFileHandle(1);
            int8_t initError = getFileHandleMember(appHandle, initErr);
            writeArgInt(0, initError);
        } else if (opcodeOffset == 0x5) {
            // killApp.
            if (!currentImplementerHasAdminPerm()) {
                throw(PERM_ERR_CODE);
            }
            allocPointer_t appHandle = readArgFileHandle(0);
            allocPointer_t runningApp = getFileHandleRunningApp(appHandle);
            if (runningApp != NULL_ALLOC_POINTER) {
                softKillApp(runningApp);
            }
        }
    } else if (opcodeCategory == 0x8) {
        // File instructions.
        if (opcodeOffset == 0x0) {
            // newFile.
            allocPointer_t fileName = readArgDynamicAlloc(0);
            if (!currentImplementerMayAccessAlloc(fileName)) {
                throw(PERM_ERR_CODE);
            }
            int32_t type = readArgInt(1);
            int8_t isGuarded = (readArgInt(2) != 0);
            int32_t size = readArgInt(3);
            createFile(fileName, type, isGuarded, size);
        } else if (opcodeOffset == 0x1) {
            // delFile.
            allocPointer_t fileHandle = readArgFileHandle(0);
            if (!currentImplementerMayAccessFile(fileHandle)) {
                throw(PERM_ERR_CODE);
            }
            deleteFile(fileHandle);
        } else if (opcodeOffset == 0x2) {
            // openFile.
            allocPointer_t fileName = readArgDynamicAlloc(1);
            if (!currentImplementerMayAccessAlloc(fileName)) {
                throw(PERM_ERR_CODE);
            }
            allocPointer_t fileHandle = openFileByStringAlloc(fileName);
            checkUnhandledError();
            if (fileHandle == NULL_ALLOC_POINTER) {
                throw(MISSING_ERR_CODE);
            }
            writeArgInt(0, fileHandle);
        } else if (opcodeOffset == 0x3) {
            // closeFile.
            allocPointer_t fileHandle = readArgFileHandle(0);
            closeFile(fileHandle);
        } else if (opcodeOffset == 0x4) {
            // readFile.
            instructionArg_t *destination = instructionArgArray;
            allocPointer_t fileHandle = readArgFileHandle(1);
            if (!currentImplementerMayAccessFile(fileHandle)) {
                throw(PERM_ERR_CODE);
            }
            int32_t pos = readArgInt(2);
            int32_t size = readArgInt(3);
            validateArgBounds(destination, size);
            checkUnhandledError();
            validateFileRange(fileHandle, pos, size);
            storageOffset_t contentAddress = getFileHandleDataAddress(fileHandle) + pos;
            for (storageOffset_t offset = 0; offset < size; offset++) {
                int8_t value = readStorageSpace(contentAddress + offset, int8_t);
                writeArgIntHelper2(destination, offset, SIGNED_INT_8_TYPE, value);
                checkUnhandledError();
            }
        } else if (opcodeOffset == 0x5) {
            // wrtFile.
            allocPointer_t fileHandle = readArgFileHandle(0);
            if (!currentImplementerMayAccessFile(fileHandle)) {
                throw(PERM_ERR_CODE);
            }
            int32_t pos = readArgInt(1);
            instructionArg_t *source = instructionArgArray + 2;
            int32_t size = readArgInt(3);
            validateArgBounds(source, size);
            checkUnhandledError();
            validateFileRange(fileHandle, pos, size);
            storageOffset_t contentAddress = getFileHandleDataAddress(fileHandle) + pos;
            for (storageOffset_t offset = 0; offset < size; offset++) {
                int8_t value = readArgIntHelper2(source, offset, SIGNED_INT_8_TYPE);
                checkUnhandledError();
                writeStorageSpace(contentAddress + offset, int8_t, value);
            }
        }
    } else if (opcodeCategory == 0x9) {
        // File metadata instructions.
        if (opcodeOffset == 0x0) {
            // allFileNames.
            allocPointer_t nameArray = getAllFileNames();
            checkUnhandledError();
            writeArgInt(0, nameArray);
        } else if (opcodeOffset == 0x1) {
            // fileExists.
            allocPointer_t fileName = readArgDynamicAlloc(1);
            if (!currentImplementerMayAccessAlloc(fileName)) {
                throw(PERM_ERR_CODE);
            }
            heapMemoryOffset_t nameAddress = getDynamicAllocDataAddress(fileName);
            heapMemoryOffset_t nameSize = getDynamicAllocSize(fileName);
            storageOffset_t fileAddress = getFileAddressByName(nameAddress, nameSize);
            writeArgInt(0, (fileAddress != MISSING_FILE_ADDRESS));
        } else if (opcodeOffset == 0x2) {
            // fileName.
            allocPointer_t fileHandle = readArgFileHandle(1);
            uint8_t nameSize = getFileHandleMember(fileHandle, nameSize);
            allocPointer_t nameAlloc = createDynamicAlloc(
                nameSize,
                GUARDED_ALLOC_ATTR,
                currentImplementer
            );
            checkUnhandledError();
            storageOffset_t fileAddress = getFileHandleMember(fileHandle, address);
            copyStorageNameToMemory(
                getFileNameAddress(fileAddress),
                getDynamicAllocDataAddress(nameAlloc),
                nameSize
            );
            writeArgInt(0, nameAlloc);
        } else if (opcodeOffset == 0x3) {
            // fileType.
            allocPointer_t fileHandle = readArgFileHandle(1);
            int8_t fileType = getFileHandleType(fileHandle);
            writeArgInt(0, fileType);
        } else if (opcodeOffset == 0x4) {
            // fileIsGuarded.
            allocPointer_t fileHandle = readArgFileHandle(1);
            int8_t attributes = getFileHandleMember(fileHandle, attributes);
            int8_t isGuarded = getIsGuardedFromFileAttributes(attributes);
            writeArgInt(0, isGuarded);
        } else if (opcodeOffset == 0x5) {
            // fileSize.
            allocPointer_t fileHandle = readArgFileHandle(1);
            storageOffset_t size = getFileHandleMember(fileHandle, contentSize);
            writeArgInt(0, size);
        }
    } else if (opcodeCategory == 0xA) {
        // Permission instructions.
        if (opcodeOffset == 0x0) {
            // hasAdminPerm.
            allocPointer_t fileHandle = readArgFileHandle(1);
            int8_t attributes = getFileHandleMember(fileHandle, attributes);
            int8_t hasAdminPerm = getHasAdminPermFromFileAttributes(attributes);
            writeArgInt(0, hasAdminPerm);
        } else if (opcodeOffset == 0x1) {
            // giveAdminPerm.
            if (!currentImplementerHasAdminPerm()) {
                throw(PERM_ERR_CODE);
            }
            allocPointer_t fileHandle = readArgFileHandle(0);
            setFileHasAdminPerm(fileHandle, true);
        } else if (opcodeOffset == 0x2) {
            // delAdminPerm.
            if (!currentImplementerHasAdminPerm()) {
                throw(PERM_ERR_CODE);
            }
            allocPointer_t fileHandle = readArgFileHandle(0);
            setFileHasAdminPerm(fileHandle, false);
        }
    } else if (opcodeCategory == 0xB) {
        // Resource instructions.
        if (opcodeOffset == 0x0) {
            // memSize.
            writeArgInt(0, HEAP_MEMORY_SIZE);
        } else if (opcodeOffset == 0x1) {
            // appMemSize.
            allocPointer_t runningApp = readArgRunningApp(1);
            heapMemoryOffset_t memoryUsage = 0;
            allocPointer_t pointer = firstAlloc;
            while (pointer != NULL_ALLOC_POINTER) {
                allocPointer_t owner = getAllocOwner(pointer);
                if (owner == runningApp) {
                    memoryUsage += getAllocSizeWithHeader(pointer);
                }
                pointer = getAllocNext(pointer);
            }
            writeArgInt(0, memoryUsage);
        } else if (opcodeOffset == 0x2) {
            // memSizeLeft.
            writeArgInt(0, heapMemorySizeLeft);
        } else if (opcodeOffset == 0x3) {
            // volSize.
            writeArgInt(0, STORAGE_SPACE_SIZE);
        } else if (opcodeOffset == 0x4) {
            // volSizeLeft.
            storageOffset_t storageUsage = sizeof(storageSpaceHeader_t);
            storageOffset_t address = getFirstFileAddress();
            while (address != MISSING_FILE_ADDRESS) {
                storageUsage += getFileStorageSize(address);
                address = getFileHeaderMember(address, next);
            }
            writeArgInt(0, STORAGE_SPACE_SIZE - storageUsage);
        }
    }
}

void resetSystemState() {
    firstAlloc = NULL_ALLOC_POINTER;
    heapMemorySizeLeft = HEAP_MEMORY_SIZE;
    firstThread = NULL_ALLOC_POINTER;
    firstRunningApp = NULL_ALLOC_POINTER;
    killStatesDelay = 0;
    unhandledErrorCode = NONE_ERR_CODE;
}


