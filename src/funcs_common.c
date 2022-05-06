
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
        startAddress = endAddress + sizeof(allocHeader_t) + getAllocSize(nextPointer);
        previousPointer = nextPointer;
        nextPointer = getAllocNext(nextPointer);
    }
    
    // Return null if there is not enough free memory.
    if (startAddress + sizeWithHeader > HEAP_MEMORY_SIZE) {
        return NULL_ALLOC_POINTER;
    }
    
    // Set up output allocation.
    allocPointer_t output = convertAddressToPointer(startAddress);
    setAllocMember(output, type, type);
    setAllocMember(output, size, size);
    setAllocMember(output, next, nextPointer);
    
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
    if (previousPointer == NULL_ALLOC_POINTER) {
        firstAlloc = nextPointer;
    } else {
        setAllocMember(previousPointer, next, nextPointer);
    }
    
    return true;
}

void validateAllocPointer(allocPointer_t pointer) {
    if (pointer == NULL_ALLOC_POINTER) {
        unhandledErrorCode = NULL_ERR_CODE;
        return;
    }
    allocPointer_t tempAlloc = firstAlloc;
    while (tempAlloc != NULL_ALLOC_POINTER) {
        if (tempAlloc == pointer) {
            return;
        }
        tempAlloc = getAllocNext(tempAlloc);
    }
    unhandledErrorCode = PTR_ERR_CODE;
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
    for (heapMemoryOffset_t index = 0; index < size; index++) {
        int8_t character = readFixedArrayElement(fixedArray, index);
        writeDynamicAlloc(output, index, int8_t, character);
    }
    return output;
}

void validateDynamicAlloc(allocPointer_t pointer) {
    validateAllocPointer(pointer);
    if (unhandledErrorCode != 0) {
        return;
    }
    if (getAllocType(pointer) != DYNAMIC_ALLOC_TYPE) {
        unhandledErrorCode = TYPE_ERR_CODE;
        return;
    }
}

int8_t memoryNameEqualsStorageName(
    heapMemoryOffset_t memoryNameAddress,
    uint8_t memoryNameSize,
    int32_t storageNameAddress,
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
    int32_t storageNameAddress,
    heapMemoryOffset_t memoryNameAddress,
    uint8_t nameSize
) {
    for (uint8_t index = 0; index < nameSize; index++) {
        int8_t tempCharacter = readStorageSpace(storageNameAddress + index, int8_t);
        writeHeapMemory(memoryNameAddress + index, int8_t, tempCharacter);
    }
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
            getFileHandleMember(tempPointer, address) + sizeof(fileHeader_t),
            getFileHandleMember(tempPointer, nameSize)
        )) {
            continue;
        }
        int8_t tempDepth = getFileHandleMember(tempPointer, openDepth);
        setFileHandleMember(tempPointer, openDepth, tempDepth + 1);
        return tempPointer;
    }
    
    // Try to find file in storage.
    int32_t fileAddress = getFirstFileAddress();
    while (true) {
        if (fileAddress == 0) {
            // File is missing.
            return NULL_ALLOC_POINTER;
        }
        if (memoryNameEqualsStorageName(
            nameAddress,
            nameSize,
            getFileNameAddress(fileAddress),
            getFileHeaderMember(fileAddress, nameSize)
        )) {
            break;
        }
        fileAddress = getFileHeaderMember(fileAddress, next);
    }
    
    // Read file header.
    uint8_t fileAttributes = getFileHeaderMember(fileAddress, attributes);
    int32_t contentSize = getFileHeaderMember(fileAddress, contentSize);
    
    // Create file handle.
    allocPointer_t output = createDynamicAlloc(
        sizeof(fileHandle_t),
        GUARDED_ALLOC_ATTR | SENTRY_ALLOC_ATTR,
        NULL_ALLOC_POINTER
    );
    setFileHandleMember(output, address, fileAddress);
    setFileHandleMember(output, attributes, fileAttributes);
    setFileHandleMember(output, nameSize, nameSize);
    setFileHandleMember(output, contentSize, contentSize);
    setFileHandleRunningApp(output, NULL_ALLOC_POINTER);
    setFileHandleInitErr(output, 0);
    setFileHandleMember(output, openDepth, 1);
    return output;
}

void closeFile(allocPointer_t fileHandle) {
    flushStorageSpace();
    int8_t openDepth = getFileHandleMember(fileHandle, openDepth);
    if (openDepth > 1) {
        setFileHandleMember(fileHandle, openDepth, openDepth - 1);
        return;
    }
    deleteAlloc(fileHandle);
}

void readFileRange(
    void *destination,
    allocPointer_t fileHandle,
    int32_t pos,
    int32_t amount
) {
    int32_t address = getFileHandleMember(fileHandle, address) + sizeof(fileHeader_t) \
        + getFileHandleMember(fileHandle, nameSize) + pos;
    readStorageSpaceRange(destination, address, amount);
}

allocPointer_t getAllFileNames() {
    
    // First find the number of files in the volume.
    int32_t fileCount = 0;
    int32_t fileAddress = getFirstFileAddress();
    while (fileAddress != 0) {
        fileCount += 1;
        fileAddress = getFileHeaderMember(fileAddress, next);
    }
    allocPointer_t output = createDynamicAlloc(
        fileCount * 4,
        GUARDED_ALLOC_ATTR,
        currentImplementerFileHandle
    );
    
    // Then create an allocation for each file name.
    fileAddress = getFirstFileAddress();
    int32_t index = 0;
    while (index < fileCount) {
        uint8_t nameSize = getFileHeaderMember(fileAddress, nameSize);
        allocPointer_t nameAlloc = createDynamicAlloc(
            nameSize,
            GUARDED_ALLOC_ATTR,
            currentImplementerFileHandle
        );
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
    if (unhandledErrorCode != 0) {
        return;
    }
    if (getDynamicAllocMember(fileHandle, creator) != NULL_ALLOC_POINTER) {
        unhandledErrorCode = TYPE_ERR_CODE;
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
    allocPointer_t output = createAlloc(ARG_FRAME_ALLOC_TYPE, size);
    setLocalFrameMember(currentLocalFrame, nextArgFrame, output);
    return output;
}

void cleanUpNextArgFrameHelper(allocPointer_t localFrame) {
    allocPointer_t nextArgFrame = getLocalFrameMember(localFrame, nextArgFrame);
    if (nextArgFrame != NULL_ALLOC_POINTER) {
        deleteAlloc(nextArgFrame);
    }
}

void launchApp(allocPointer_t fileHandle) {
    
    // Do not launch app again if it is already running.
    allocPointer_t runningApp = getFileHandleRunningApp(fileHandle);
    if (runningApp != NULL_ALLOC_POINTER) {
        return;
    }
    int8_t fileType = getFileHandleType(fileHandle);
    int32_t fileSize = getFileHandleSize(fileHandle);
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
                unhandledErrorCode = DATA_ERR_CODE;
                return;
            }
            expectedFilePos += instructionBodySize;
        }
        if (expectedFilePos != appDataFilePos || appDataFilePos > fileSize) {
            unhandledErrorCode = DATA_ERR_CODE;
            return;
        }
        
        globalFrameSize = sizeof(bytecodeGlobalFrameHeader_t) + (heapMemoryOffset_t)getBytecodeAppMember(fileHandle, globalFrameSize);
    } else if (fileType == SYSTEM_APP_FILE_TYPE) {
        
        // Validate system app file.
        if (fileSize != 1) {
            unhandledErrorCode = DATA_ERR_CODE;
            return;
        }
        
        systemAppId = readFile(fileHandle, 0, int8_t);
        globalFrameSize = sizeof(systemGlobalFrameHeader_t) + getSystemAppMember(systemAppId, globalFrameSize);
    } else {
        unhandledErrorCode = TYPE_ERR_CODE;
        return;
    }
    
    // Create allocation for the running app.
    runningApp = createAlloc(
        RUNNING_APP_ALLOC_TYPE,
        sizeof(runningAppHeader_t) + globalFrameSize
    );
    setRunningAppMember(runningApp, fileHandle, fileHandle);
    setRunningAppMember(runningApp, localFrame, NULL_ALLOC_POINTER);
    setRunningAppMember(runningApp, isWaiting, false);
    setFileHandleRunningApp(fileHandle, runningApp);
    setFileHandleInitErr(fileHandle, 0);
    
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
    int32_t initFunctionIndex = findFunctionById(runningApp, INIT_FUNC_ID);
    if (initFunctionIndex >= 0) {
        callFunction(runningApp, runningApp, initFunctionIndex);
    }
}

void hardKillApp(allocPointer_t runningApp, int8_t errorCode) {
    
    // Delete local frames and argument frames.
    allocPointer_t localFrame = getRunningAppMember(runningApp, localFrame);
    while (localFrame != NULL_ALLOC_POINTER) {
        cleanUpNextArgFrameHelper(localFrame);
        allocPointer_t previousLocalFrame = getLocalFrameMember(
            localFrame,
            previousLocalFrame
        );
        deleteAlloc(localFrame);
        localFrame = previousLocalFrame;
    }
    
    // Delete dynamic allocations.
    allocPointer_t fileHandle = getRunningAppMember(
        currentThreadApp,
        fileHandle
    );
    allocPointer_t tempAlloc = firstAlloc;
    while (tempAlloc != NULL_ALLOC_POINTER) {
        allocPointer_t nextAlloc = getAllocNext(tempAlloc);
        int8_t tempType = getAllocType(tempAlloc);
        if (tempType == DYNAMIC_ALLOC_TYPE) {
            allocPointer_t tempCreator = getDynamicAllocMember(tempAlloc, creator);
            if (tempCreator == fileHandle) {
                deleteAlloc(tempAlloc);
            }
        }
        tempAlloc = nextAlloc;
    }
    
    // Update file handle and delete running app.
    setFileHandleRunningApp(fileHandle, NULL_ALLOC_POINTER);
    setFileHandleInitErr(fileHandle, errorCode);
    closeFile(fileHandle);
    deleteAlloc(runningApp);
}

void callFunction(
    allocPointer_t threadApp,
    allocPointer_t implementer,
    int32_t functionIndex
) {
    
    allocPointer_t fileHandle = getRunningAppMember(implementer, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    allocPointer_t previousLocalFrame = getRunningAppMember(
        threadApp,
        localFrame
    );
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
        unhandledErrorCode = INDEX_ERR_CODE;
        return;
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
            unhandledErrorCode = ARG_FRAME_ERR_CODE;
            return;
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
    
    // Create allocation for the local frame.
    allocPointer_t localFrame = createAlloc(LOCAL_FRAME_ALLOC_TYPE, localFrameSize);
    setLocalFrameMember(localFrame, implementer, implementer);
    setLocalFrameMember(localFrame, functionIndex, functionIndex);
    setLocalFrameMember(localFrame, previousLocalFrame, previousLocalFrame);
    setLocalFrameMember(localFrame, nextArgFrame, NULL_ALLOC_POINTER);
    
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
    
    // Update thread app local frame.
    setRunningAppMember(threadApp, localFrame, localFrame);
}

// Does not update currentThreadApp.
void setCurrentLocalFrame(allocPointer_t localFrame) {
    currentLocalFrame = localFrame;
    currentImplementer = getLocalFrameMember(currentLocalFrame, implementer);
    currentImplementerFileHandle = getRunningAppMember(currentImplementer, fileHandle);
    currentImplementerFileType = getFileHandleType(currentImplementerFileHandle);
}

void returnFromFunction() {
    cleanUpNextArgFrame();
    allocPointer_t previousLocalFrame = getLocalFrameMember(
        currentLocalFrame,
        previousLocalFrame
    );
    deleteAlloc(currentLocalFrame);
    setRunningAppMember(currentThreadApp, localFrame, previousLocalFrame);
    setCurrentLocalFrame(previousLocalFrame);
}

void scheduleAppThread(allocPointer_t runningApp) {
    
    currentThreadApp = runningApp;
    allocPointer_t tempFrame = getRunningAppMember(
        currentThreadApp,
        localFrame
    );
    if (tempFrame == NULL_ALLOC_POINTER) {
        return;
    }
    setCurrentLocalFrame(tempFrame);
    
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
    
    if (unhandledErrorCode != 0) {
        //printDebugString((int8_t *)"UNHANDLED ERROR ");
        //printDebugNumber(unhandledErrorCode);
        //printDebugNewline();
        while (true) {
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
                setLocalFrameMember(currentLocalFrame, lastErrorCode, unhandledErrorCode);
                break;
            }
            returnFromFunction();
            if (currentLocalFrame == NULL_ALLOC_POINTER) {
                hardKillApp(currentThreadApp, unhandledErrorCode);
                break;
            }
        }
        unhandledErrorCode = 0;
    }
}

void runAppSystem() {
    
    // Launch boot application.
    allocPointer_t bootFileName = createStringAllocFromFixedArray(BOOT_STRING_CONSTANT);
    allocPointer_t bootFileHandle = openFileByStringAlloc(bootFileName);
    deleteAlloc(bootFileName);
    if (bootFileHandle == NULL_ALLOC_POINTER) {
        return;
    }
    launchApp(bootFileHandle);
    if (unhandledErrorCode != 0) {
        return;
    }
    
    // Enter loop scheduling app threads.
    int8_t runningAppIndex = 0;
    while (true) {
        
        // Find running app with index equal to runningAppIndex.
        allocPointer_t runningApp = NULL_ALLOC_POINTER;
        allocPointer_t firstRunningApp = NULL_ALLOC_POINTER;
        int8_t index = 0;
        allocPointer_t tempAlloc = firstAlloc;
        while (tempAlloc != NULL_ALLOC_POINTER) {
            int8_t tempType = getAllocType(tempAlloc);
            if (tempType == RUNNING_APP_ALLOC_TYPE) {
                if (index == 0) {
                    firstRunningApp = tempAlloc;
                }
                if (index == runningAppIndex) {
                    runningApp = tempAlloc;
                    break;
                }
                index += 1;
            }
            tempAlloc = getAllocNext(tempAlloc);
        }
        
        // If we couldn't find running app at runningAppIndex,
        // try to schedule the first running app.
        if (runningApp == NULL_ALLOC_POINTER) {
            if (firstRunningApp == NULL_ALLOC_POINTER) {
                return;
            }
            runningApp = firstRunningApp;
            runningAppIndex = 0;
        }
        
        // Schedule thread time for runningApp.
        scheduleAppThread(runningApp);
        sleepMilliseconds(1);
        runningAppIndex += 1;
    }
}

void jumpToBytecodeInstruction(int32_t instructionOffset) {
    // TODO: Implement.
}

void evaluateBytecodeInstruction() {
    // TODO: Implement.
}


