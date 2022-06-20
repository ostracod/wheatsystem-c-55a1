
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
        unhandledErrorCode = CAPACITY_ERR_CODE;
        return NULL_ALLOC_POINTER;
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
    allocPointer_t tempAlloc = firstAlloc;
    while (tempAlloc != NULL_ALLOC_POINTER) {
        if (tempAlloc == pointer) {
            return;
        }
        tempAlloc = getAllocNext(tempAlloc);
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
        int8_t tempCharacter1 = readHeapMemory(memoryNameAddress + index, int8_t);
        int8_t tempCharacter2 = readStorageSpace(storageNameAddress + index, int8_t);
        if (tempCharacter1 != tempCharacter2) {
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
        int8_t tempCharacter = readStorageSpace(storageNameAddress + index, int8_t);
        writeHeapMemory(memoryNameAddress + index, int8_t, tempCharacter);
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
        throw(NUM_RANGE_ERR_CODE);
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
        allocPointer_t tempPointer = nextPointer;
        nextPointer = getAllocNext(tempPointer);
        if (!allocIsFileHandle(tempPointer)) {
            continue;
        }
        if (!memoryNameEqualsStorageName(
            nameAddress,
            nameSize,
            getFileNameAddress(getFileHandleMember(tempPointer, address)),
            getFileHandleMember(tempPointer, nameSize)
        )) {
            continue;
        }
        int8_t tempDepth = getFileHandleMember(tempPointer, openDepth);
        setFileHandleMember(tempPointer, openDepth, tempDepth + 1);
        return tempPointer;
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
    setFileHandleRunningApp(output, NULL_ALLOC_POINTER);
    setFileHandleInitErr(output, NONE_ERR_CODE);
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

void closeFile(allocPointer_t fileHandle) {
    flushStorageSpace();
    int8_t openDepth = getFileHandleMember(fileHandle, openDepth);
    if (openDepth <= 0) {
        return;
    }
    openDepth -= 1;
    setFileHandleMember(fileHandle, openDepth, openDepth - 1);
    if (openDepth <= 0) {
        deleteFileHandle(fileHandle);
    }
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
        // TODO: Clean up garbage allocations if we have an error.
        checkUnhandledError(NULL_ALLOC_POINTER);
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
    heapMemoryOffset_t tempAddress = getDynamicAllocDataAddress(stringAlloc);
    heapMemoryOffset_t tempSize = getDynamicAllocSize(stringAlloc);
    return openFile(tempAddress, (uint8_t)tempSize);
}

int8_t allocIsFileHandle(allocPointer_t pointer) {
    return (getAllocType(pointer) == DYNAMIC_ALLOC_TYPE
        && getDynamicAllocMember(pointer, creator) == NULL_ALLOC_POINTER
        && (getDynamicAllocMember(pointer, attributes) & SENTRY_ALLOC_ATTR));
}

void validateFileHandle(allocPointer_t fileHandle) {
    validateDynamicAlloc(fileHandle);
    checkUnhandledError();
    if (getDynamicAllocMember(fileHandle, creator) != NULL_ALLOC_POINTER) {
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
            int32_t tempFunctionId = getBytecodeFunctionMember(fileHandle, index, functionId);
            if (tempFunctionId == functionId) {
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
    storageOffset_t fileSize = getFileHandleSize(fileHandle);
    int32_t functionTableLength;
    int32_t appDataFilePos;
    int8_t systemAppId;
    
    // Determine global frame size.
    heapMemoryOffset_t globalFrameSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        
        // Validate bytecode app file.
        functionTableLength = getBytecodeAppMember(fileHandle, functionTableLength);
        appDataFilePos = getBytecodeAppMember(fileHandle, appDataFilePos);
        int32_t expectedFilePos = sizeof(bytecodeAppHeader_t) + functionTableLength * sizeof(bytecodeFunction_t);
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
    setRunningAppMember(runningApp, isWaiting, false);
    setRunningAppMember(runningApp, killAction, NONE_KILL_ACTION);
    setRunningAppMember(runningApp, previous, NULL_ALLOC_POINTER);
    setRunningAppMember(runningApp, next, firstRunningApp);
    if (firstRunningApp != NULL_ALLOC_POINTER) {
        setRunningAppMember(firstRunningApp, previous, runningApp);
    }
    firstRunningApp = runningApp;
    setFileHandleRunningApp(fileHandle, runningApp);
    setFileHandleInitErr(fileHandle, NONE_ERR_CODE);
    
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
    allocPointer_t tempAlloc = firstAlloc;
    while (tempAlloc != NULL_ALLOC_POINTER) {
        allocPointer_t nextAlloc = getAllocNext(tempAlloc);
        int8_t tempType = getAllocType(tempAlloc);
        if (tempType == DYNAMIC_ALLOC_TYPE) {
            allocPointer_t tempCreator = getDynamicAllocMember(tempAlloc, creator);
            if (tempCreator == runningApp) {
                deleteAlloc(tempAlloc);
            }
        }
        tempAlloc = nextAlloc;
    }
    
    // Update file handle and delete running app.
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    setFileHandleRunningApp(fileHandle, NULL_ALLOC_POINTER);
    setFileHandleInitErr(fileHandle, errorCode);
    closeFile(fileHandle);
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
    int32_t functionAmount;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        functionAmount = getBytecodeGlobalFrameMember(implementer, functionTableLength);
    } else {
        int8_t systemAppId = getSystemGlobalFrameMember(implementer, id);
        systemAppFunctionList = getSystemAppMember(systemAppId, functionList);
        functionAmount = getSystemAppMember(systemAppId, functionAmount);
    }
    if (functionIndex < 0 || functionIndex >= functionAmount) {
        throw(INDEX_ERR_CODE);
    }
    
    // Validate current implementer permission.
    if (shouldCheckPerm && currentImplementer != implementer
            && !currentImplementerHasAdminPerm()) {
        int8_t isGuarded;
        if (fileType == BYTECODE_APP_FILE_TYPE) {
            isGuarded = getBytecodeFunctionMember(fileHandle, functionIndex, isGuarded);
        } else {
            isGuarded = getSystemAppFunctionListMember(
                systemAppFunctionList,
                functionIndex,
                isGuarded
            );
        }
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
    heapMemoryOffset_t localFrameSize = sizeof(localFrameHeader_t);
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        localFrameSize += sizeof(bytecodeLocalFrameHeader_t) + (heapMemoryOffset_t)getBytecodeFunctionMember(fileHandle, functionIndex, localFrameSize);
    } else {
        localFrameSize += getSystemAppFunctionListMember(
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
    allocPointer_t localFrame = createAlloc(LOCAL_FRAME_ALLOC_TYPE, localFrameSize);
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
    allocPointer_t previousThread = getThreadMember(thread, previous);
    allocPointer_t nextThread = getThreadMember(thread, next);
    if (previousThread == NULL_ALLOC_POINTER) {
        firstThread = nextThread;
    } else {
        setThreadMember(previousThread, next, nextThread);
    }
    if (nextThread != NULL_ALLOC_POINTER) {
        setThreadMember(nextThread, previous, previousThread);
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
                setFileHandleInitErr(fileHandle, error);
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
    // currentThread will be null if the app quits while running.
    if (unhandledErrorCode != NONE_ERR_CODE && currentThread != NULL_ALLOC_POINTER) {
        registerErrorInCurrentThread(unhandledErrorCode);
        unhandledErrorCode = NONE_ERR_CODE;
    }
}

void runAppSystem() {
    
    // Launch boot application.
    allocPointer_t bootFileName = createStringAllocFromFixedArray(bootStringConstant);
    checkUnhandledError();
    allocPointer_t bootFileHandle = openFileByStringAlloc(bootFileName);
    deleteAlloc(bootFileName);
    checkUnhandledError();
    if (bootFileHandle == NULL_ALLOC_POINTER) {
        return;
    }
    launchApp(bootFileHandle);
    checkUnhandledError();
    nextThread = firstThread;
    
    // Enter loop scheduling app threads.
    while (nextThread != NULL_ALLOC_POINTER) {
        killStatesDelay += 1;
        if (killStatesDelay >= KILL_STATES_PERIOD) {
            updateKillStates();
            killStatesDelay = 0;
        }
        setCurrentThread(nextThread);
        advanceNextThread(currentThread);
        if (currentLocalFrame == NULL_ALLOC_POINTER) {
            deleteThread(currentThread);
        } else {
            scheduleCurrentThread();
        }
        sleepMilliseconds(1);
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

int8_t currentImplementerMayAccessAlloc(allocPointer_t dynamicAlloc) {
    int8_t attributes = getDynamicAllocMember(dynamicAlloc, attributes);
    if ((attributes & GUARDED_ALLOC_ATTR) == 0) {
        return true;
    }
    allocPointer_t creator = getDynamicAllocMember(dynamicAlloc, creator);
    return (creator == currentImplementer || currentImplementerHasAdminPerm());
}

int8_t currentImplementerMayAccessFile(allocPointer_t fileHandle) {
    if (currentImplementerHasAdminPerm()) {
        return true;
    }
    int8_t attributes = getFileHandleMember(fileHandle, attributes);
    return !getIsGuardedFromFileAttributes(attributes);
}

void setFileHasAdminPerm(allocPointer_t fileHandle, int8_t hasAdminPerm) {
    if (!currentImplementerHasAdminPerm()) {
        throw(PERM_ERR_CODE);
    }
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

heapMemoryOffset_t getArgHeapMemoryAddress(
    instructionArg_t *arg,
    int32_t offset,
    int8_t dataTypeSize
) {
    heapMemoryOffset_t index = arg->index + offset;
    if (index < 0 || index + dataTypeSize > arg->size) {
        unhandledErrorCode = INDEX_ERR_CODE;
    }
    return arg->startAddress + index;
}

int32_t getArgBufferSize(instructionArg_t *arg) {
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    if (referenceType == CONSTANT_REF_TYPE) {
        return -1;
    } else if (referenceType == APP_DATA_REF_TYPE) {
        int32_t appDataSize = getBytecodeGlobalFrameMember(currentImplementer, appDataSize);
        return appDataSize - arg->appDataIndex;
    } else {
        return arg->size - arg->index;
    }
}

int32_t readArgIntHelper(instructionArg_t *arg, int32_t offset, int8_t dataType) {
    uint8_t tempPrefix = arg->prefix;
    uint8_t referenceType = getArgPrefixReferenceType(tempPrefix);
    if (dataType < 0) {
        dataType = getArgPrefixDataType(tempPrefix);
    }
    int8_t dataTypeSize = getArgDataTypeSize(dataType);
    if (referenceType == CONSTANT_REF_TYPE) {
        return arg->constantValue;
    } else if (referenceType == APP_DATA_REF_TYPE) {
        int32_t index = arg->appDataIndex + offset;
        int32_t appDataSize = getBytecodeGlobalFrameMember(currentImplementer, appDataSize);
        if (index < 0 || index + dataTypeSize > appDataSize) {
            unhandledErrorCode = INDEX_ERR_CODE;
            return 0;
        }
        int32_t tempFilePos = getBytecodeGlobalFrameMember(
            currentImplementer,
            appDataFilePos
        ) + index;
        if (dataType == SIGNED_INT_8_TYPE) {
            return readFile(currentImplementerFileHandle, tempFilePos, int8_t);
        } else {
            return readFile(currentImplementerFileHandle, tempFilePos, int32_t);
        }
    } else {
        heapMemoryOffset_t tempAddress = getArgHeapMemoryAddress(arg, offset, dataTypeSize);
        checkUnhandledError(0);
        if (dataType == SIGNED_INT_8_TYPE) {
            return readHeapMemory(tempAddress, int8_t);
        } else {
            return readHeapMemory(tempAddress, int32_t);
        }
    }
}

void writeArgIntHelper(
    instructionArg_t *arg,
    int32_t offset,
    int8_t dataType,
    int32_t value
) {
    uint8_t tempPrefix = arg->prefix;
    uint8_t referenceType = getArgPrefixReferenceType(tempPrefix);
    if (dataType < 0) {
        dataType = getArgPrefixDataType(tempPrefix);
    }
    int8_t dataTypeSize = getArgDataTypeSize(dataType);
    if (referenceType == HEAP_MEM_REF_TYPE) {
        heapMemoryOffset_t tempAddress = getArgHeapMemoryAddress(arg, offset, dataTypeSize);
        checkUnhandledError();
        if (dataType == SIGNED_INT_8_TYPE) {
            writeHeapMemory(tempAddress, int8_t, (int8_t)value);
        } else {
            writeHeapMemory(tempAddress, int32_t, value);
        }
    } else {
        throw(TYPE_ERR_CODE);
    }
}

int32_t readArgConstantIntHelper(int8_t index) {
    instructionArg_t *tempArg = instructionArgArray + index;
    uint8_t referenceType = getArgPrefixReferenceType(tempArg->prefix);
    if (referenceType != CONSTANT_REF_TYPE) {
        unhandledErrorCode = TYPE_ERR_CODE;
    }
    return tempArg->constantValue;
}

void readArgRunningAppHelper(allocPointer_t *destination, int8_t index) {
    allocPointer_t appHandle = readArgFileHandle(index);
    allocPointer_t runningApp = getFileHandleRunningApp(appHandle);
    if (runningApp == NULL_ALLOC_POINTER) {
        if (getFileHandleType(runningApp) == GENERIC_FILE_TYPE) {
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
    if (dataType > SIGNED_INT_32_TYPE) {
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
        instructionArg_t tempArg1;
        parseInstructionArg(&tempArg1);
        checkUnhandledError();
        int32_t argValue1 = readArgIntHelper(&tempArg1, 0, -1);
        checkUnhandledError();
        if (referenceType == APP_DATA_REF_TYPE) {
            destination->prefix = argPrefix;
            destination->appDataIndex = argValue1;
        } else {
            heapMemoryOffset_t startAddress;
            heapMemoryOffset_t tempSize;
            heapMemoryOffset_t index;
            if (referenceType == DYNAMIC_ALLOC_REF_TYPE) {
                validateDynamicAlloc(argValue1);
                checkUnhandledError();
                if (!currentImplementerMayAccessAlloc(argValue1)) {
                    throw(PERM_ERR_CODE);
                }
                startAddress = getDynamicAllocDataAddress(argValue1);
                instructionArg_t tempArg2;
                parseInstructionArg(&tempArg2);
                checkUnhandledError();
                index = (heapMemoryOffset_t)readArgIntHelper(&tempArg2, 0, -1);
                checkUnhandledError();
                tempSize = getDynamicAllocSize(argValue1);
            } else {
                index = (heapMemoryOffset_t)argValue1;
                if (referenceType == GLOBAL_FRAME_REF_TYPE) {
                    startAddress = getBytecodeGlobalFrameDataAddress(currentImplementer);
                    tempSize = getBytecodeGlobalFrameSize(currentImplementer);
                } else if (referenceType == LOCAL_FRAME_REF_TYPE) {
                    startAddress = getBytecodeLocalFrameDataAddress(currentLocalFrame);
                    tempSize = getBytecodeLocalFrameSize(currentLocalFrame);
                } else {
                    allocPointer_t tempLocalFrame;
                    if (referenceType == PREV_ARG_FRAME_REF_TYPE) {
                        tempLocalFrame = getLocalFrameMember(
                            currentLocalFrame,
                            previousLocalFrame
                        );
                        if (tempLocalFrame == NULL_ALLOC_POINTER) {
                            throw(ARG_FRAME_ERR_CODE);
                        }
                    } else if (referenceType == NEXT_ARG_FRAME_REF_TYPE) {
                        tempLocalFrame = currentLocalFrame;
                    } else {
                        throw(TYPE_ERR_CODE);
                    }
                    allocPointer_t argFrame = getLocalFrameMember(
                        tempLocalFrame,
                        nextArgFrame
                    );
                    if (argFrame == NULL_ALLOC_POINTER) {
                        throw(ARG_FRAME_ERR_CODE);
                    }
                    startAddress = getArgFrameDataAddress(argFrame);
                    tempSize = getArgFrameSize(argFrame);
                }
            }
            destination->prefix = (HEAP_MEM_REF_TYPE << 4) | dataType;
            destination->startAddress = startAddress;
            destination->size = tempSize;
            destination->index = index;
        }
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
    uint8_t opcodeOffset = opcode & 0x0F;
    int8_t tempOffset = readFixedArrayElement(argumentAmountOffsetArray, opcodeCategory);
    int8_t argumentAmount = readFixedArrayElement(
        argumentAmountArray,
        tempOffset + opcodeOffset
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
    if (opcodeCategory == 0x0) {
        // Memory instructions.
        if (opcodeOffset == 0x0) {
            // wrt.
            int32_t tempValue = readArgInt(1);
            writeArgInt(0, tempValue);
        } else if (opcodeOffset == 0x1) {
            // wrtBuff.
            instructionArg_t *destination = instructionArgArray;
            instructionArg_t *source = instructionArgArray + 1;
            int32_t size = readArgInt(2);
            int32_t destinationBufferSize = getArgBufferSize(destination);
            int32_t sourceBufferSize = getArgBufferSize(source);
            if (size < 0 || size > destinationBufferSize || size > sourceBufferSize) {
                throw(NUM_RANGE_ERR_CODE);
            }
            for (int32_t offset = 0; offset < size; offset++) {
                int8_t tempValue = readArgIntHelper(source, offset, SIGNED_INT_8_TYPE);
                checkUnhandledError();
                writeArgIntHelper(destination, offset, SIGNED_INT_8_TYPE, tempValue);
                checkUnhandledError();
            }
        } else if (opcodeOffset == 0x2) {
            // newArgFrame.
            heapMemoryOffset_t argFrameSize = (heapMemoryOffset_t)readArgInt(0);
            if (argFrameSize < 0) {
                throw(NUM_RANGE_ERR_CODE);
            }
            createNextArgFrame(argFrameSize);
        } else if (opcodeOffset == 0x3) {
            // newAlloc.
            int8_t tempAttributes = (int8_t)readArgInt(1) & ALLOC_ATTR_MASK;
            heapMemoryOffset_t tempSize = (heapMemoryOffset_t)readArgInt(2);
            if (tempSize < 0) {
                throw(NUM_RANGE_ERR_CODE);
            }
            allocPointer_t tempAlloc = createDynamicAlloc(
                tempSize,
                tempAttributes,
                currentImplementer
            );
            checkUnhandledError();
            writeArgInt(0, tempAlloc);
        } else if (opcodeOffset == 0x4) {
            // delAlloc.
            allocPointer_t tempAlloc = readArgDynamicAlloc(0);
            if (!currentImplementerMayAccessAlloc(tempAlloc)) {
                throw(PERM_ERR_CODE);
            }
            deleteAlloc(tempAlloc);
        } else if (opcodeOffset == 0x5) {
            // allocAttrs.
            allocPointer_t tempAlloc = readArgDynamicAlloc(1);
            int8_t tempAttributes = getDynamicAllocMember(tempAlloc, attributes);
            writeArgInt(0, tempAttributes);
        } else if (opcodeOffset == 0x6) {
            // allocSize.
            allocPointer_t tempAlloc = readArgDynamicAlloc(1);
            heapMemoryOffset_t tempSize = getDynamicAllocSize(tempAlloc);
            writeArgInt(0, tempSize);
        } else if (opcodeOffset == 0x7) {
            // allocCreator.
            allocPointer_t tempAlloc = readArgDynamicAlloc(1);
            allocPointer_t tempCreator = getDynamicAllocMember(tempAlloc, creator);
            allocPointer_t fileHandle = getRunningAppMember(tempCreator, fileHandle);
            writeArgInt(0, fileHandle);
        } else if (opcodeOffset == 0x8) {
            // setAllocAttrs.
            allocPointer_t alloc = readArgDynamicAlloc(0);
            if (!currentImplementerMayAccessAlloc(alloc)) {
                throw(PERM_ERR_CODE);
            }
            int8_t attributes = readArgInt(1) & ALLOC_ATTR_MASK;
            setDynamicAllocMember(alloc, attributes, attributes);
        } else {
            throw(NO_IMPL_ERR_CODE);
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
        } else {
            throw(NO_IMPL_ERR_CODE);
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
            int32_t tempCode = readArgInt(0);
            if (tempCode < -128 || tempCode > 127) {
                throw(NUM_RANGE_ERR_CODE);
            }
            throw(tempCode);
        } else if (opcodeOffset == 0x3) {
            // err.
            int8_t tempCode = getLocalFrameMember(currentLocalFrame, lastErrorCode);
            writeArgInt(0, tempCode);
        } else {
            throw(NO_IMPL_ERR_CODE);
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
            allocPointer_t tempImplementer = readArgRunningApp(0);
            int32_t functionIndex = readArgInt(1);
            callFunction(currentThread, tempImplementer, functionIndex, true);
        } else if (opcodeOffset == 0x3) {
            // ret.
            returnFromFunction();
        } else if (opcodeOffset == 0x4) {
            // caller.
            allocPointer_t tempCaller = getCurrentCaller();
            allocPointer_t fileHandle;
            if (tempCaller == NULL_ALLOC_POINTER) {
                fileHandle = NULL_ALLOC_POINTER;
            } else {
                fileHandle = getRunningAppMember(tempCaller, fileHandle);
            }
            writeArgInt(0, fileHandle);
        } else {
            throw(NO_IMPL_ERR_CODE);
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
            } else {
                throw(NO_IMPL_ERR_CODE);
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
        } else {
            throw(NO_IMPL_ERR_CODE);
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
        } else {
            throw(NO_IMPL_ERR_CODE);
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
            int8_t initError = getFileHandleInitErr(appHandle);
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
        } else {
            throw(NO_IMPL_ERR_CODE);
        }
    } else if (opcodeCategory == 0x8) {
        // File instructions.
        if (opcodeOffset == 0x0) {
            // newFile.
            allocPointer_t fileName = readArgDynamicAlloc(0);
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
            validateFileRange(fileHandle, pos, size);
            storageOffset_t contentAddress = getFileHandleDataAddress(fileHandle) + pos;
            for (storageOffset_t offset = 0; offset < size; offset++) {
                int8_t value = readStorageSpace(contentAddress + offset, int8_t);
                writeArgIntHelper(destination, offset, SIGNED_INT_8_TYPE, value);
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
            validateFileRange(fileHandle, pos, size);
            storageOffset_t contentAddress = getFileHandleDataAddress(fileHandle) + pos;
            for (storageOffset_t offset = 0; offset < size; offset++) {
                int8_t value = readArgIntHelper(source, offset, SIGNED_INT_8_TYPE);
                checkUnhandledError();
                writeStorageSpace(contentAddress + offset, int8_t, value);
            }
        } else {
            throw(NO_IMPL_ERR_CODE);
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
            storageOffset_t size = getFileHandleSize(fileHandle);
            writeArgInt(0, size);
        } else {
            throw(NO_IMPL_ERR_CODE);
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
            allocPointer_t fileHandle = readArgFileHandle(0);
            setFileHasAdminPerm(fileHandle, true);
        } else if (opcodeOffset == 0x2) {
            // delAdminPerm.
            allocPointer_t fileHandle = readArgFileHandle(0);
            setFileHasAdminPerm(fileHandle, false);
        } else {
            throw(NO_IMPL_ERR_CODE);
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
        } else {
            throw(NO_IMPL_ERR_CODE);
        }
    } else {
        throw(NO_IMPL_ERR_CODE);
    }
}


