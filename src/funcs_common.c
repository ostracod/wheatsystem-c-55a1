
#include "./headers.h"

int8_t getSpanSizeDegree(heapMemOffset_t size) {
    uint16_t adjustedSize = (uint16_t)size + (uint16_t)sizeof(spanHeader_t) + (uint16_t)64;
    int8_t exponent = 9;
    uint16_t value = (uint16_t)64 << exponent;
    while (value > adjustedSize) {
        value >>= 1;
        exponent -= 1;
    }
    return (exponent << 2) + ((adjustedSize - value) >> (exponent + 4));
}

void initializeEmptySpan(heapMemOffset_t address, heapMemOffset_t size) {
    int8_t degree = getSpanSizeDegree(size);
    heapMemOffset_t nextAddress = emptySpansByDegree[degree];
    setEmptySpanMember(address, previousByDegree, MISSING_SPAN_ADDRESS);
    setEmptySpanMember(address, nextByDegree, nextAddress);
    setEmptySpanMember(address, degree, degree);
    if (nextAddress != MISSING_SPAN_ADDRESS) {
        setEmptySpanMember(nextAddress, previousByDegree, address);
    }
    emptySpansByDegree[degree] = address;
    emptySpanBitField[degree >> 3] |= 1 << (degree & 0x07);
}

void cleanUpEmptySpan(heapMemOffset_t address) {
    heapMemOffset_t previousAddress = getEmptySpanMember(address, previousByDegree);
    heapMemOffset_t nextAddress = getEmptySpanMember(address, nextByDegree);
    int8_t degree = getEmptySpanMember(address, degree);
    if (previousAddress == MISSING_SPAN_ADDRESS) {
        emptySpansByDegree[degree] = nextAddress;
        if (nextAddress == MISSING_SPAN_ADDRESS) {
            emptySpanBitField[degree >> 3] &= ~(1 << (degree & 0x07));
        }
    } else {
        setEmptySpanMember(previousAddress, nextByDegree, nextAddress);
    }
    if (nextAddress != MISSING_SPAN_ADDRESS) {
        setEmptySpanMember(nextAddress, previousByDegree, previousAddress);
    }
}

allocPointer_t createAlloc(int8_t type, heapMemOffset_t size) {
    
    // Find a gap which is large enough for the new allocation.
    heapMemOffset_t sizeWithHeader = sizeof(allocHeader_t) + size;
    int8_t degree = getSpanSizeDegree(sizeWithHeader - 1) + 1;
    int8_t fieldIndex = degree >> 3;
    int8_t fieldValue = emptySpanBitField[fieldIndex];
    if ((fieldValue & (0xFF << (degree & 0x07))) == 0) {
        while (true) {
            fieldIndex += 1;
            if (fieldIndex >= SPAN_BIT_FIELD_SIZE) {
                throw(CAPACITY_ERR_CODE, NULL_ALLOC_POINTER);
            }
            fieldValue = emptySpanBitField[fieldIndex];
            if (fieldValue != 0) {
                break;
            }
        }
        degree = fieldIndex << 3;
    }
    uint8_t mask = 0x01 << (degree & 0x07);
    while ((fieldValue & mask) == 0) {
        degree += 1;
        mask <<= 1;
    }
    heapMemOffset_t spanAddress1 = emptySpansByDegree[degree];
    heapMemOffset_t nextSpanAddress = getSpanMember(spanAddress1, nextByNeighbor);
    heapMemOffset_t spanSize1 = getSpanMember(spanAddress1, size);
    heapMemOffset_t tempAddress = getEmptySpanMember(spanAddress1, nextByDegree);
    emptySpansByDegree[degree] = tempAddress;
    if (tempAddress != MISSING_SPAN_ADDRESS) {
        setEmptySpanMember(tempAddress, previousByDegree, MISSING_SPAN_ADDRESS);
    }
    cleanUpEmptySpan(spanAddress1);
    
    // Split span if enough space is left over.
    heapMemOffset_t splitSize = getMaximum(sizeWithHeader, MINIMUM_SPAN_SIZE);
    heapMemOffset_t spanAddress2 = ((spanAddress1 + sizeof(spanHeader_t) + splitSize - 1) & ~((heapMemOffset_t)SPAN_ALIGNMENT - 1)) + SPAN_ALIGNMENT;
    heapMemOffset_t spanSize2 = spanAddress1 + spanSize1 - spanAddress2;
    if (spanSize2 > MINIMUM_SPAN_SIZE) {
        setSpanMember(spanAddress2, previousByNeighbor, spanAddress1);
        setSpanMember(spanAddress2, nextByNeighbor, nextSpanAddress);
        setSpanMember(spanAddress2, size, spanSize2);
        setSpanMember(spanAddress2, allocType, NONE_ALLOC_TYPE);
        initializeEmptySpan(spanAddress2, spanSize2);
        if (nextSpanAddress != MISSING_SPAN_ADDRESS) {
            setSpanMember(nextSpanAddress, previousByNeighbor, spanAddress2);
        }
        spanSize1 = (spanAddress2 - spanAddress1) - sizeof(spanHeader_t);
        setSpanMember(spanAddress1, nextByNeighbor, spanAddress2);
    } else {
        setSpanMember(spanAddress1, nextByNeighbor, nextSpanAddress);
        if (nextSpanAddress != MISSING_SPAN_ADDRESS) {
            setSpanMember(nextSpanAddress, previousByNeighbor, spanAddress1);
        }
    }
    
    // Set up remaining span members.
    // Note that previousByNeighbor is already set correctly.
    setSpanMember(spanAddress1, size, spanSize1);
    setSpanMember(spanAddress1, allocType, type);
    allocPointer_t output = getSpanAllocPointer(spanAddress1);
    setAllocMember(output, previousByType, NULL_ALLOC_POINTER);
    allocPointer_t nextAlloc = allocsByType[type];
    allocsByType[type] = output;
    setAllocMember(output, nextByType, nextAlloc);
    setAllocMember(output, size, size);
    if (nextAlloc != NULL_ALLOC_POINTER) {
        setAllocMember(nextAlloc, previousByType, output);
    }
    heapMemSizeLeft -= sizeof(spanHeader_t) + spanSize1;
    
    // Update the allocation bit field.
    int16_t truncatedAddress = spanAddress1 >> SPAN_ALIGNMENT_EXPONENT;
    allocBitField[truncatedAddress >> 3] |= (uint8_t)1 << (truncatedAddress & 7);
    
    return output;
}

void deleteAlloc(allocPointer_t pointer) {
    
    // Update allocation linked list.
    allocPointer_t previousAlloc = getAllocMember(pointer, previousByType);
    allocPointer_t nextAlloc = getAllocNextByType(pointer);
    if (previousAlloc == NULL_ALLOC_POINTER) {
        heapMemOffset_t type = getAllocType(pointer);
        allocsByType[type] = nextAlloc;
    } else {
        setAllocMember(previousAlloc, nextByType, nextAlloc);
    }
    if (nextAlloc != NULL_ALLOC_POINTER) {
        setAllocMember(nextAlloc, previousByType, previousAlloc);
    }
    
    // Retrieve span members.
    heapMemOffset_t address = getAllocSpanAddress(pointer);
    heapMemOffset_t previousAddress = getSpanMember(address, previousByNeighbor);
    heapMemOffset_t nextAddress = getSpanMember(address, nextByNeighbor);
    heapMemOffset_t size = getSpanMember(address, size);
    heapMemSizeLeft += sizeof(spanHeader_t) + size;
    heapMemOffset_t linkAddress1 = address;
    heapMemOffset_t linkAddress2 = nextAddress;
    
    // Merge with previous empty span if it exists.
    if (previousAddress != MISSING_SPAN_ADDRESS) {
        int8_t previousType = getSpanMember(previousAddress, allocType);
        if (previousType == NONE_ALLOC_TYPE) {
            linkAddress1 = previousAddress;
            cleanUpEmptySpan(previousAddress);
        }
    }
    
    // Merge with next empty span if it exists.
    if (nextAddress != MISSING_SPAN_ADDRESS) {
        int8_t nextType = getSpanMember(nextAddress, allocType);
        if (nextType == NONE_ALLOC_TYPE) {
            linkAddress2 = getSpanMember(nextAddress, nextByNeighbor);
            cleanUpEmptySpan(nextAddress);
        }
    }
    
    // Update span linked lists and allocation type if necessary.
    heapMemOffset_t endAddress;
    if (linkAddress2 == MISSING_SPAN_ADDRESS) {
        endAddress = HEAP_MEM_SIZE;
    } else {
        endAddress = linkAddress2;
    }
    heapMemOffset_t newSize = (endAddress - linkAddress1) - sizeof(spanHeader_t);
    int8_t addressIsEqual = (linkAddress1 == address);
    if (addressIsEqual) {
        setSpanMember(linkAddress1, allocType, NONE_ALLOC_TYPE);
    }
    if (!addressIsEqual || linkAddress2 != nextAddress) {
        setSpanMember(linkAddress1, nextByNeighbor, linkAddress2);
        setSpanMember(linkAddress1, size, newSize);
        if (linkAddress2 != MISSING_SPAN_ADDRESS) {
            setSpanMember(linkAddress2, previousByNeighbor, linkAddress1);
        }
    }
    initializeEmptySpan(linkAddress1, newSize);
    
    // Update the allocation bit field.
    int16_t truncatedAddress = address >> SPAN_ALIGNMENT_EXPONENT;
    allocBitField[truncatedAddress >> 3] &= ~((uint8_t)1 << (truncatedAddress & 7));
}

void validateAllocPointer(int32_t pointer) {
    if (pointer < 0 || pointer >= HEAP_MEM_SIZE) {
        throw(PTR_ERR_CODE);
    }
    if (pointer == NULL_ALLOC_POINTER) {
        throw(NULL_ERR_CODE);
    }
    heapMemOffset_t address = getAllocSpanAddress(pointer);
    if ((address & (SPAN_ALIGNMENT - 1)) != 0) {
        throw(PTR_ERR_CODE);
    }
    int16_t truncatedAddress = address >> SPAN_ALIGNMENT_EXPONENT;
    uint8_t result = allocBitField[truncatedAddress >> 3] & ((uint8_t)1 << (truncatedAddress & 7));
    if (result == 0) {
        throw(PTR_ERR_CODE);
    }
}

allocPointer_t createDynamicAlloc(
    heapMemOffset_t size,
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
    for (heapMemOffset_t index = 0; index < size; index++) {
        writeDynamicAlloc(output, index, int8_t, 0);
    }
    return output;
}

allocPointer_t createStringAllocFromFixedArrayHelper(
    const int8_t *fixedArray,
    heapMemOffset_t size
) {
    allocPointer_t output = createDynamicAlloc(size, GUARDED_ALLOC_ATTR, NULL_ALLOC_POINTER);
    checkUnhandledError(NULL_ALLOC_POINTER);
    for (heapMemOffset_t index = 0; index < size; index++) {
        int8_t character = readFixedArrayElement(fixedArray, index);
        writeDynamicAlloc(output, index, int8_t, character);
    }
    return output;
}

void validateDynamicAlloc(int32_t dynamicAlloc) {
    validateAllocPointer(dynamicAlloc);
    checkUnhandledError();
    if (getAllocType(dynamicAlloc) != DYNAMIC_ALLOC_TYPE) {
        throw(TYPE_ERR_CODE);
    }
}

allocPointer_t createSystemSentry(int8_t type, heapMemOffset_t size) {
    allocPointer_t output = createDynamicAlloc(
        sizeof(systemSentryHeader_t) + size,
        GUARDED_ALLOC_ATTR | SENTRY_ALLOC_ATTR,
        NULL_ALLOC_POINTER
    );
    checkUnhandledError(NULL_ALLOC_POINTER);
    setSystemSentryMember(output, type, type);
    return output;
}

int8_t dynamicAllocIsSystemSentry(allocPointer_t dynamicAlloc, int8_t type) {
    return ((getDynamicAllocMember(dynamicAlloc, attributes) & SENTRY_ALLOC_ATTR)
        && getDynamicAllocMember(dynamicAlloc, creator) == NULL_ALLOC_POINTER
        && getSystemSentryMember(dynamicAlloc, type) == type);
}

void validateSystemSentry(int32_t sentry, int8_t type) {
    validateDynamicAlloc(sentry);
    checkUnhandledError();
    if (!dynamicAllocIsSystemSentry(sentry, type)) {
        throw(TYPE_ERR_CODE);
    }
}

allocPointer_t createGate(int8_t mode) {
    allocPointer_t output = createSystemSentry(GATE_SENTRY_TYPE, sizeof(gateSentry_t));
    checkUnhandledError(NULL_ALLOC_POINTER);
    setGateMember(output, owner, currentImplementer);
    setGateMember(output, mode, mode);
    setGateMember(output, isOpen, true);
    return output;
}

void deleteGate(allocPointer_t gate) {
    allocPointer_t lastThread = currentThread;
    allocPointer_t thread = allocsByType[THREAD_ALLOC_TYPE];
    while (thread != NULL_ALLOC_POINTER) {
        if (getThreadMember(thread, blockingGate) == gate) {
            setCurrentThread(thread);
            registerErrorInCurrentThread(MISSING_ERR_CODE);
        }
        thread = getAllocNextByType(thread);
    }
    setCurrentThread(lastThread);
    deleteAlloc(gate);
}

int8_t passThroughGate(allocPointer_t gate) {
    if (getGateMember(gate, mode) == PASS_CLOSE_GATE_MODE) {
        setGateMember(gate, isOpen, false);
        return true;
    } else {
        return false;
    }
}

void waitForGate(allocPointer_t gate) {
    if (getGateMember(gate, isOpen)) {
        passThroughGate(gate);
    } else {
        setThreadMember(currentThread, blockingGate, gate);
        shouldStopCoalescence = true;
    }
}

void openGate(allocPointer_t gate) {
    setGateMember(gate, isOpen, true);
    allocPointer_t thread = allocsByType[THREAD_ALLOC_TYPE];
    while (thread != NULL_ALLOC_POINTER) {
        if (getThreadMember(thread, blockingGate) == gate) {
            setThreadMember(thread, blockingGate, NULL_ALLOC_POINTER);
            int8_t hasClosed = passThroughGate(gate);
            if (hasClosed) {
                break;
            }
        }
        thread = getAllocNextByType(thread);
    }
}

void validateGate(int32_t gate) {
    validateSystemSentry(gate, GATE_SENTRY_TYPE);
    checkUnhandledError();
    if (getGateMember(gate, owner) != currentImplementer) {
        throw(DATA_ERR_CODE);
    }
}

int8_t heapMemNameEqualsStorageName(
    heapMemOffset_t heapMemNameAddress,
    uint8_t heapMemNameSize,
    storageOffset_t storageNameAddress,
    uint8_t storageNameSize
) {
    if (heapMemNameSize != storageNameSize) {
        return false;
    }
    uint8_t index = 0;
    while (index < heapMemNameSize) {
        int8_t buffer1[NAME_COMPARISON_STRIDE];
        int8_t buffer2[NAME_COMPARISON_STRIDE];
        uint8_t sizeToCompare = heapMemNameSize - index;
        if (sizeToCompare > NAME_COMPARISON_STRIDE) {
            sizeToCompare = NAME_COMPARISON_STRIDE;
        }
        readHeapMemRange(buffer1, heapMemNameAddress + index, sizeToCompare);
        readStorageRange(buffer2, storageNameAddress + index, sizeToCompare);
        for (uint8_t offset = 0; offset < sizeToCompare; offset++) {
            int8_t character1 = buffer1[offset];
            int8_t character2 = buffer2[offset];
            if (character1 != character2) {
                return false;
            }
        }
        index += sizeToCompare;
    }
    return true;
}

void copyStorageNameToHeapMem(
    storageOffset_t storageNameAddress,
    heapMemOffset_t heapMemNameAddress,
    uint8_t nameSize
) {
    uint8_t index = 0;
    while (index < nameSize) {
        int8_t buffer[BUFFER_COPY_STRIDE];
        uint8_t sizeToCopy = nameSize - index;
        if (sizeToCopy > BUFFER_COPY_STRIDE) {
            sizeToCopy = BUFFER_COPY_STRIDE;
        }
        readStorageRange(buffer, storageNameAddress + index, sizeToCopy);
        writeHeapMemRange(heapMemNameAddress + index, buffer, sizeToCopy);
        index += sizeToCopy;
    }
}

void createFile(
    allocPointer_t name,
    int8_t type,
    int8_t isGuarded,
    storageOffset_t contentSize
) {
    
    // Verify argument values.
    heapMemOffset_t nameAllocAddress = getDynamicAllocDataAddress(name);
    heapMemOffset_t nameSize = getDynamicAllocSize(name);
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
                startAddress = sizeof(storageHeader_t);
            } else {
                startAddress = previousFileAddress + getFileStorageSize(previousFileAddress);
            }
            storageOffset_t endAddress = hasReachedEnd ? STORAGE_SIZE : nextFileAddress;
            if (endAddress - startAddress >= fileStorageSize) {
                newFileAddress = startAddress;
            }
        }
        if (hasReachedEnd) {
            break;
        }
        if (heapMemNameEqualsStorageName(
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
        setStorageMember(firstFileAddress, newFileAddress);
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
    heapMemOffset_t nameHeapMemAddress = getDynamicAllocDataAddress(name);
    storageOffset_t nameStorageAddress = newFileAddress + sizeof(fileHeader_t);
    uint8_t offset = 0;
    while (offset < nameSize) {
        int8_t buffer[BUFFER_COPY_STRIDE];
        uint8_t sizeToCopy = nameSize - offset;
        if (sizeToCopy > BUFFER_COPY_STRIDE) {
            sizeToCopy = BUFFER_COPY_STRIDE;
        }
        readHeapMemRange(buffer, nameHeapMemAddress + offset, sizeToCopy);
        writeStorageRange(nameStorageAddress + offset, buffer, sizeToCopy);
        offset += sizeToCopy;
    }
    storageOffset_t contentAddress = nameStorageAddress + nameSize;
    for (storageOffset_t offset = 0; offset < contentSize; offset++) {
        writeStorage(contentAddress + offset, int8_t, 0);
    }
    flushStorage();
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
        setStorageMember(firstFileAddress, nextFileAddress);
    } else {
        setFileHeaderMember(previousFileAddress, next, nextFileAddress);
    }
    flushStorage();
    deleteFileHandle(fileHandle);
}

storageOffset_t getFileAddressByName(
    heapMemOffset_t nameAddress,
    heapMemOffset_t nameSize
) {
    storageOffset_t fileAddress = getFirstFileAddress();
    while (fileAddress != MISSING_FILE_ADDRESS) {
        if (heapMemNameEqualsStorageName(
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

void incrementFileOpenDepth(allocPointer_t fileHandle) {
    int8_t depth = getFileHandleMember(fileHandle, openDepth);
    setFileHandleMember(fileHandle, openDepth, depth + 1);
}

allocPointer_t openFile(heapMemOffset_t nameAddress, heapMemOffset_t nameSize) {
    
    // Return matching file handle if it already exists.
    allocPointer_t fileHandle = firstFileHandle;
    while (fileHandle != NULL_ALLOC_POINTER) {
        if (heapMemNameEqualsStorageName(
            nameAddress,
            nameSize,
            getFileNameAddress(getFileHandleMember(fileHandle, address)),
            getFileHandleMember(fileHandle, nameSize)
        )) {
            incrementFileOpenDepth(fileHandle);
            return fileHandle;
        }
        fileHandle = getFileHandleMember(fileHandle, next);
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
    allocPointer_t output = createSystemSentry(FILE_HANDLE_SENTRY_TYPE, sizeof(fileHandle_t));
    checkUnhandledError(NULL_ALLOC_POINTER);
    setSystemSentryMember(output, type, FILE_HANDLE_SENTRY_TYPE);
    setFileHandleMember(output, address, fileAddress);
    setFileHandleMember(output, attributes, fileAttributes);
    setFileHandleMember(output, nameSize, nameSize);
    setFileHandleMember(output, contentSize, contentSize);
    setFileHandleMember(output, previous, NULL_ALLOC_POINTER);
    setFileHandleMember(output, next, firstFileHandle);
    setFileHandleMember(output, runningApp, NULL_ALLOC_POINTER);
    setFileHandleMember(output, initErr, NONE_ERR_CODE);
    setFileHandleMember(output, openDepth, 1);
    if (firstFileHandle != NULL_ALLOC_POINTER) {
        setFileHandleMember(firstFileHandle, previous, output);
    }
    firstFileHandle = output;
    return output;
}

void deleteFileHandle(allocPointer_t fileHandle) {
    allocPointer_t previousFileHandle = getFileHandleMember(fileHandle, previous);
    allocPointer_t nextFileHandle = getFileHandleMember(fileHandle, next);
    if (previousFileHandle == NULL_ALLOC_POINTER) {
        firstFileHandle = nextFileHandle;
    } else {
        setFileHandleMember(previousFileHandle, next, nextFileHandle);
    }
    if (nextFileHandle != NULL_ALLOC_POINTER) {
        setFileHandleMember(nextFileHandle, previous, previousFileHandle);
    }
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
    flushStorage();
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
    readStorageRange(destination, address, amount);
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
        copyStorageNameToHeapMem(
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
    heapMemOffset_t address = getDynamicAllocDataAddress(stringAlloc);
    heapMemOffset_t size = getDynamicAllocSize(stringAlloc);
    return openFile(address, (uint8_t)size);
}

int32_t findFuncById(allocPointer_t runningApp, int32_t funcId) {
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        int32_t funcTableLength = getBytecodeGlobalFrameMember(
            runningApp,
            funcTableLength
        );
        for (int32_t index = 0; index < funcTableLength; index++) {
            int32_t tempId = getBytecodeFuncMember(fileHandle, index, funcId);
            if (tempId == funcId) {
                return index;
            }
        }
    } else if (fileType == SYSTEM_APP_FILE_TYPE) {
        int8_t systemAppId = getSystemGlobalFrameMember(runningApp, id);
        const systemAppFunc_t *funcList = getSystemAppMember(
            systemAppId,
            funcList
        );
        int8_t funcAmount = getSystemAppMember(systemAppId, funcAmount);
        for (int8_t index = 0; index < funcAmount; index++) {
            int8_t tempId = getSystemAppFuncListMember(funcList, index, id);
            if (tempId == funcId) {
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

allocPointer_t createNextArgFrame(heapMemOffset_t size) {
    cleanUpNextArgFrame();
    allocPointer_t output = createAlloc(
        ARG_FRAME_ALLOC_TYPE,
        sizeof(argFrameHeader_t) + size
    );
    if (unhandledErrorCode != NONE_ERR_CODE) {
        setLocalFrameMember(currentLocalFrame, nextArgFrame, NULL_ALLOC_POINTER);
        return NULL_ALLOC_POINTER;
    }
    setArgFrameMember(output, thread, currentThread);
    for (heapMemOffset_t index = 0; index < size; index++) {
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
    int32_t funcTableLength;
    int32_t appDataFilePos;
    int8_t systemAppId;
    
    // Determine global frame size.
    heapMemOffset_t globalFrameSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        
        // Validate bytecode app file.
        if (fileSize < sizeof(bytecodeAppHeader_t)) {
            throw(DATA_ERR_CODE);
        }
        funcTableLength = getBytecodeAppMember(fileHandle, funcTableLength);
        appDataFilePos = getBytecodeAppMember(fileHandle, appDataFilePos);
        int32_t expectedFilePos = sizeof(bytecodeAppHeader_t) + funcTableLength * sizeof(bytecodeFunc_t);
        if (expectedFilePos > fileSize) {
            throw(DATA_ERR_CODE);
        }
        for (int32_t index = 0; index < funcTableLength; index++) {
            int32_t instructionBodyFilePos = getBytecodeFuncMember(
                fileHandle,
                index,
                instructionBodyFilePos
            );
            int32_t instructionBodySize = getBytecodeFuncMember(
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
        
        globalFrameSize = sizeof(bytecodeGlobalFrameHeader_t) + (heapMemOffset_t)getBytecodeAppMember(fileHandle, globalFrameSize);
    } else if (fileType == SYSTEM_APP_FILE_TYPE) {
        
        // Validate system app file.
        if (fileSize != 1) {
            throw(DATA_ERR_CODE);
        }
        systemAppId = readFile(fileHandle, 0, int8_t);
        if (systemAppId < 0 || systemAppId >= getArrayLength(systemAppArray)) {
            throw(DATA_ERR_CODE);
        }
        
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
    setRunningAppMember(runningApp, killAction, NONE_KILL_ACTION);
    setFileHandleMember(fileHandle, runningApp, runningApp);
    setFileHandleMember(fileHandle, initErr, NONE_ERR_CODE);
    
    // Initialize global frame.
    for (heapMemOffset_t index = 0; index < globalFrameSize; index++) {
        writeGlobalFrame(runningApp, index, int8_t, 0);
    }
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        setBytecodeGlobalFrameMember(runningApp, funcTableLength, funcTableLength);
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
    allocPointer_t thread = allocsByType[THREAD_ALLOC_TYPE];
    while (thread != NULL_ALLOC_POINTER) {
        allocPointer_t tempThread = thread;
        thread = getAllocNextByType(tempThread);
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
    thread = allocsByType[THREAD_ALLOC_TYPE];
    while (thread != NULL_ALLOC_POINTER) {
        allocPointer_t tempThread = thread;
        thread = getAllocNextByType(tempThread);
        allocPointer_t bottomFrame = getBottomLocalFrame(tempThread, runningApp);
        if (bottomFrame == NULL_ALLOC_POINTER) {
            continue;
        }
        setCurrentThread(tempThread);
        while (currentLocalFrame != NULL_ALLOC_POINTER) {
            allocPointer_t localFrame = currentLocalFrame;
            returnFromFunc();
            updateLocalFrameContext();
            if (localFrame == bottomFrame) {
                break;
            }
        }
        registerErrorInCurrentThread(STATE_ERR_CODE);
    }
    setCurrentThread(lastThread);
    
    // Delete dynamic allocations.
    allocPointer_t alloc = allocsByType[DYNAMIC_ALLOC_TYPE];
    while (alloc != NULL_ALLOC_POINTER) {
        allocPointer_t nextAlloc = getAllocNextByType(alloc);
        int8_t type = getAllocType(alloc);
        if (type == DYNAMIC_ALLOC_TYPE) {
            allocPointer_t creator = getDynamicAllocMember(alloc, creator);
            if (creator == runningApp || (dynamicAllocIsSystemSentry(alloc, GATE_SENTRY_TYPE)
                    && getGateMember(alloc, owner) == runningApp)) {
                deleteAlloc(alloc);
            }
        }
        alloc = nextAlloc;
    }
    
    // Update file handle and delete running app.
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    setFileHandleMember(fileHandle, runningApp, NULL_ALLOC_POINTER);
    deleteFileHandleIfUnused(fileHandle);
    deleteAlloc(runningApp);
}

int8_t funcIsGuarded(allocPointer_t implementer, int32_t funcIndex) {
    allocPointer_t fileHandle = getRunningAppMember(implementer, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    const systemAppFunc_t *systemAppFuncList;
    validateFuncIndex(implementer, fileType, funcIndex, systemAppFuncList, false);
    int8_t output;
    funcIsGuardedHelper(
        output,
        fileHandle,
        fileType,
        funcIndex,
        systemAppFuncList
    );
    return output;
}

void callFunc(
    allocPointer_t thread,
    allocPointer_t implementer,
    int32_t funcIndex,
    int8_t shouldCheckPerm
) {
    
    allocPointer_t fileHandle = getRunningAppMember(implementer, fileHandle);
    int8_t fileType = getFileHandleType(fileHandle);
    allocPointer_t previousLocalFrame = getThreadMember(thread, localFrame);
    const systemAppFunc_t *systemAppFuncList;
    
    // Validate function index.
    validateFuncIndex(implementer, fileType, funcIndex, systemAppFuncList);
    
    // Validate current implementer permission.
    if (shouldCheckPerm && currentImplementer != implementer
            && !currentImplementerHasAdminPerm()) {
        int8_t isGuarded;
        funcIsGuardedHelper(
            isGuarded,
            fileHandle,
            fileType,
            funcIndex,
            systemAppFuncList
        );
        if (isGuarded) {
            throw(PERM_ERR_CODE);
        }
    }
    
    // Validate arg frame size.
    int32_t argFrameSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        argFrameSize = getBytecodeFuncMember(fileHandle, funcIndex, argFrameSize);
    } else {
        argFrameSize = getSystemAppFuncListMember(
            systemAppFuncList,
            funcIndex,
            argFrameSize
        );
    }
    allocPointer_t previousArgFrame = NULL_ALLOC_POINTER;
    if (argFrameSize > 0) {
        if (previousLocalFrame != NULL_ALLOC_POINTER) {
            previousArgFrame = getLocalFrameMember(previousLocalFrame, nextArgFrame);
        }
        if (previousArgFrame == NULL_ALLOC_POINTER
                || getArgFrameSize(previousArgFrame) != argFrameSize) {
            throw(ARG_FRAME_ERR_CODE);
        }
    }
    
    // Determine local frame size.
    heapMemOffset_t localVarsStartIndex;
    heapMemOffset_t localVarsSize;
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        localVarsStartIndex = sizeof(bytecodeLocalFrameHeader_t);
        localVarsSize = (heapMemOffset_t)getBytecodeFuncMember(fileHandle, funcIndex, localFrameSize);
    } else {
        localVarsStartIndex = 0;
        localVarsSize = getSystemAppFuncListMember(
            systemAppFuncList,
            funcIndex,
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
    setLocalFrameMember(localFrame, funcIndex, funcIndex);
    setLocalFrameMember(localFrame, previousLocalFrame, previousLocalFrame);
    setLocalFrameMember(localFrame, previousArgFrame, previousArgFrame);
    setLocalFrameMember(localFrame, nextArgFrame, NULL_ALLOC_POINTER);
    setLocalFrameMember(localFrame, lastErrorCode, NONE_ERR_CODE);
    setLocalFrameMember(localFrame, shouldThrottle, shouldThrottle);
    
    if (fileType == BYTECODE_APP_FILE_TYPE) {
        // Initialize members specific to bytecode functions.
        int32_t instructionBodyFilePos = getBytecodeFuncMember(
            fileHandle,
            funcIndex,
            instructionBodyFilePos
        );
        int32_t instructionBodySize = getBytecodeFuncMember(
            fileHandle,
            funcIndex,
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
    for (heapMemOffset_t offset = 0; offset < localVarsSize; offset++) {
        writeLocalFrame(localFrame, localVarsStartIndex + offset, int8_t, 0);
    }
    
    // Update thread local frame.
    setThreadMember(thread, localFrame, localFrame);
    currentLocalFrame = localFrame;
}

void updateLocalFrameContext() {
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

void returnFromFunc() {
    cleanUpNextArgFrame();
    allocPointer_t previousLocalFrame = getLocalFrameMember(
        currentLocalFrame,
        previousLocalFrame
    );
    deleteAlloc(currentLocalFrame);
    setThreadMember(currentThread, localFrame, previousLocalFrame);
    currentLocalFrame = previousLocalFrame;
}

int8_t createThread(allocPointer_t runningApp, int32_t funcId) {
    int32_t funcIndex = findFuncById(runningApp, funcId);
    if (funcIndex < 0) {
        return false;
    }
    allocPointer_t thread = createAlloc(THREAD_ALLOC_TYPE, sizeof(thread_t));
    checkUnhandledError(true);
    setThreadMember(thread, runningApp, runningApp);
    setThreadMember(thread, funcId, funcId);
    setThreadMember(thread, localFrame, NULL_ALLOC_POINTER);
    setThreadMember(thread, blockingGate, NULL_ALLOC_POINTER);
    callFunc(thread, runningApp, funcIndex, false);
    if (nextThread == NULL_ALLOC_POINTER) {
        nextThread = thread;
    }
    return true;
}

void deleteThread(allocPointer_t thread) {
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
            setThreadMember(currentThread, blockingGate, NULL_ALLOC_POINTER);
            break;
        }
        returnFromFunc();
        updateLocalFrameContext();
        if (currentLocalFrame == NULL_ALLOC_POINTER) {
            if (getThreadMember(currentThread, funcId) == INIT_FUNC_ID) {
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

void scheduleCurrentThread() {
    if (currentImplementerFileType == BYTECODE_APP_FILE_TYPE) {
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
        shouldStopCoalescence = false;
        allocPointer_t lastLocalFrame = currentLocalFrame;
        int8_t count = 1;
        while (true) {
            evaluateBytecodeInstruction();
            if (count > 4 || currentLocalFrame != lastLocalFrame
                    || shouldStopCoalescence || unhandledErrorCode != NONE_ERR_CODE) {
                break;
            }
            count += 1;
        }
    } else {
        int8_t funcIndex = (int8_t)getLocalFrameMember(currentLocalFrame, funcIndex);
        void (*threadAction)() = getRunningSystemAppFuncMember(
            currentImplementer,
            funcIndex,
            threadAction
        );
        threadAction();
    }
    // At this point, local frame context may be dirty because some calls to "returnFromFunc"
    // are intentionally not followed by "updateLocalFrameContext".
    if (unhandledErrorCode != NONE_ERR_CODE) {
        if (currentLocalFrame != NULL_ALLOC_POINTER) {
            updateLocalFrameContext();
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
    nextThread = allocsByType[THREAD_ALLOC_TYPE];
    
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
        if (getThreadMember(thread, blockingGate) == NULL_ALLOC_POINTER) {
            setCurrentThread(thread);
            if (currentLocalFrame == NULL_ALLOC_POINTER) {
                deleteThread(currentThread);
            } else {
                scheduleCurrentThread();
                sleepAfterSchedule();
            }
        }
    }
}

int8_t runningAppHasAdminPerm(allocPointer_t runningApp) {
    allocPointer_t fileHandle = getRunningAppMember(runningApp, fileHandle);
    int8_t attributes = getFileHandleMember(fileHandle, attributes);
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
    flushStorage();
}

void iterateOverAllocs(void *context, allocIterationHandle_t handle) {
    allocPointer_t alloc;
    alloc = allocsByType[RUNNING_APP_ALLOC_TYPE];
    while (alloc != NULL_ALLOC_POINTER) {
        (*handle)(context, alloc, alloc);
        alloc = getAllocNextByType(alloc);
    }
    alloc = allocsByType[THREAD_ALLOC_TYPE];
    while (alloc != NULL_ALLOC_POINTER) {
        allocPointer_t owner = getThreadMember(alloc, runningApp);
        (*handle)(context, alloc, owner);
        alloc = getAllocNextByType(alloc);
    }
    alloc = allocsByType[LOCAL_FRAME_ALLOC_TYPE];
    while (alloc != NULL_ALLOC_POINTER) {
        allocPointer_t thread = getLocalFrameMember(alloc, thread);
        allocPointer_t owner = getThreadMember(thread, runningApp);
        (*handle)(context, alloc, owner);
        alloc = getAllocNextByType(alloc);
    }
    alloc = allocsByType[ARG_FRAME_ALLOC_TYPE];
    while (alloc != NULL_ALLOC_POINTER) {
        allocPointer_t thread = getArgFrameMember(alloc, thread);
        allocPointer_t owner = getThreadMember(thread, runningApp);
        (*handle)(context, alloc, owner);
        alloc = getAllocNextByType(alloc);
    }
    alloc = allocsByType[DYNAMIC_ALLOC_TYPE];
    while (alloc != NULL_ALLOC_POINTER) {
        allocPointer_t owner = getDynamicAllocMember(alloc, creator);
        (*handle)(context, alloc, owner);
        alloc = getAllocNextByType(alloc);
    }
}

void increaseRunningAppMemUsage(void *context, allocPointer_t alloc, allocPointer_t owner) {
    if (owner != NULL_ALLOC_POINTER) {
        heapMemOffset_t memUsage = getRunningAppMember(owner, memUsage);
        memUsage += getAllocMemUsage(alloc);
        setRunningAppMember(owner, memUsage, memUsage);
    }
}

void registerRunningAppMemUsage(
    memUsageContext_t *context,
    allocPointer_t alloc,
    allocPointer_t owner
) {
    if (owner == context->runningApp) {
        context->memUsage += getAllocMemUsage(alloc);
    }
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
        allocPointer_t thread = allocsByType[THREAD_ALLOC_TYPE];
        while (thread != NULL_ALLOC_POINTER) {
            setCurrentThread(thread);
            thread = getAllocNextByType(currentThread);
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

void updateKillStates() {
    
    // Advance states of apps which are currently being killed.
    int16_t killActionCount = 0;
    allocPointer_t runningApp;
    allocPointer_t nextRunningApp = allocsByType[RUNNING_APP_ALLOC_TYPE];
    while (nextRunningApp != NULL_ALLOC_POINTER) {
        runningApp = nextRunningApp;
        nextRunningApp = getAllocNextByType(runningApp);
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
    if (killActionCount > 0 || heapMemSizeLeft > KILL_PANIC_THRESHOLD) {
        return;
    }
    
    // Calculate memory usage for all running apps.
    runningApp = allocsByType[RUNNING_APP_ALLOC_TYPE];
    while (runningApp != NULL_ALLOC_POINTER) {
        setRunningAppMember(runningApp, memUsage, 0);
        runningApp = getAllocNextByType(runningApp);
    }
    iterateOverAllocs(NULL, increaseRunningAppMemUsage);
    
    // Determine the best app to kill.
    allocPointer_t victimRunningApp = NULL_ALLOC_POINTER;
    allocPointer_t victimMemUsage = 0;
    allocPointer_t victimHasAdminPerm = true;
    runningApp = allocsByType[RUNNING_APP_ALLOC_TYPE];
    while (runningApp != NULL_ALLOC_POINTER) {
        heapMemOffset_t memUsage = getRunningAppMember(runningApp, memUsage);
        int8_t hasAdminPerm = runningAppHasAdminPerm(runningApp);
        if ((hasAdminPerm == victimHasAdminPerm && memUsage > victimMemUsage)
                || (!hasAdminPerm && victimHasAdminPerm)) {
            victimRunningApp = runningApp;
            victimMemUsage = memUsage;
            victimHasAdminPerm = hasAdminPerm;
        }
        runningApp = getAllocNextByType(runningApp);
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
        index = 0;
        uint8_t dataType = getArgPrefixDataType(arg->prefix);
        regionSize = getArgDataTypeSize(dataType);
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

void readArgRange(
    int8_t *destination,
    instructionArg_t *arg,
    int32_t offset,
    uint8_t amount
) {
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    if (referenceType == CONSTANT_REF_TYPE) {
        // This only works because int8_t and int32_t representations
        // of the same integer have the same first byte.
        for (uint8_t index = 0; index < amount; index++) {
            destination[index] = ((int8_t *)&(arg->constantValue))[index];
        }
    } else if (referenceType == APP_DATA_REF_TYPE) {
        int32_t filePos = getBytecodeGlobalFrameMember(
            currentImplementer,
            appDataFilePos
        ) + arg->appDataIndex + offset;
        readFileRange(destination, currentImplementerFileHandle, filePos, amount);
    } else {
        heapMemOffset_t address = arg->startAddress + arg->index + offset;
        readHeapMemRange(destination, address, amount);
    }
}

void writeArgRange(
    instructionArg_t *arg,
    int32_t offset,
    int8_t *source,
    uint8_t amount
) {
    uint8_t referenceType = getArgPrefixReferenceType(arg->prefix);
    if (referenceType == HEAP_MEM_REF_TYPE) {
        heapMemOffset_t address = arg->startAddress + arg->index + offset;
        writeHeapMemRange(address, source, amount);
    } else {
        throw(TYPE_ERR_CODE);
    }
}

int32_t readArgIntHelper(instructionArg_t *arg) {
    uint8_t dataType = getArgPrefixDataType(arg->prefix);
    int8_t dataTypeSize = getArgDataTypeSize(dataType);
    int8_t buffer[4];
    validateArgBounds(arg, dataTypeSize);
    checkUnhandledError(0);
    readArgRange(buffer, arg, 0, dataTypeSize);
    if (dataType == SIGNED_INT_8_TYPE) {
        return *(int8_t *)buffer;
    } else if (dataType == SIGNED_INT_16_TYPE) {
        return *(int16_t *)buffer;
    } else {
        return *(int32_t *)buffer;
    }
}

void writeArgIntHelper(instructionArg_t *arg, int32_t value) {
    uint8_t dataType = getArgPrefixDataType(arg->prefix);
    int8_t dataTypeSize = getArgDataTypeSize(dataType);
    validateArgBounds(arg, dataTypeSize);
    checkUnhandledError();
    writeArgRange(arg, 0, (int8_t *)&value, dataTypeSize);
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
    currentInstructionFilePos = instructionBodyFilePos + instructionOffset;
    setBytecodeLocalFrameMember(
        currentLocalFrame,
        instructionFilePos,
        currentInstructionFilePos
    );
}

void parseInstructionArg(instructionArg_t *destination) {
    uint8_t argPrefix = readInstructionData(uint8_t);
    uint8_t referenceType = getArgPrefixReferenceType(argPrefix);
    uint8_t dataType = getArgPrefixDataType(argPrefix);
    if (dataType > SIGNED_INT_32_TYPE || referenceType > PREV_ARG_REF_TYPE) {
        throw(TYPE_ERR_CODE);
    }
    if (referenceType == CONSTANT_REF_TYPE) {
        destination->prefix = argPrefix;
        if (dataType == SIGNED_INT_8_TYPE) {
            destination->constantValue = readInstructionData(int8_t);
        } else if (dataType == SIGNED_INT_16_TYPE) {
            destination->constantValue = readInstructionData(int16_t);
        } else {
            destination->constantValue = readInstructionData(int32_t);
        }
    } else if (referenceType == PREV_ARG_REF_TYPE) {
        if (argParseIndex <= 0) {
            throw(MISSING_ERR_CODE);
        }
        *destination = instructionArgArray[argParseIndex - 1];
    } else {
        instructionArg_t arg1;
        parseInstructionArg(&arg1);
        checkUnhandledError();
        int32_t argValue1 = readArgIntHelper(&arg1);
        checkUnhandledError();
        if (referenceType == APP_DATA_REF_TYPE) {
            destination->prefix = argPrefix;
            destination->appDataIndex = argValue1;
        } else {
            heapMemOffset_t startAddress;
            heapMemOffset_t size;
            heapMemOffset_t index;
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
                index = (heapMemOffset_t)readArgIntHelper(&arg2);
                checkUnhandledError();
                size = getDynamicAllocSize(argValue1);
            } else {
                index = (heapMemOffset_t)argValue1;
                if (referenceType == GLOBAL_FRAME_REF_TYPE) {
                    startAddress = getBytecodeGlobalFrameDataAddress(currentImplementer);
                    size = getBytecodeGlobalFrameSize(currentImplementer);
                } else if (referenceType == LOCAL_FRAME_REF_TYPE) {
                    startAddress = getBytecodeLocalFrameDataAddress(currentLocalFrame);
                    size = getBytecodeLocalFrameSize(currentLocalFrame);
                } else {
                    allocPointer_t argFrame;
                    if (referenceType == PREV_ARG_FRAME_REF_TYPE) {
                        argFrame = getLocalFrameMember(currentLocalFrame, previousArgFrame);
                    } else {
                        // referenceType must equal NEXT_ARG_FRAME_REF_TYPE.
                        argFrame = getLocalFrameMember(currentLocalFrame, nextArgFrame);
                    }
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
        heapMemOffset_t destStartAddress = destination->startAddress + destination->index;
        heapMemOffset_t sourceStartAddress = source->startAddress + source->index;
        shouldCopyBackward = (destStartAddress > sourceStartAddress);
    } else {
        shouldCopyBackward = false;
    }
    if (shouldCopyBackward) {
        heapMemOffset_t offset = size;
        while (offset > 0) {
            int8_t buffer[BUFFER_COPY_STRIDE];
            heapMemOffset_t sizeToCopy = offset;
            if (sizeToCopy > BUFFER_COPY_STRIDE) {
                sizeToCopy = BUFFER_COPY_STRIDE;
            }
            offset -= sizeToCopy;
            readArgRange(buffer, source, offset, sizeToCopy);
            writeArgRange(destination, offset, buffer, sizeToCopy);
            checkUnhandledError();
        }
    } else {
        heapMemOffset_t offset = 0;
        while (offset < size) {
            int8_t buffer[BUFFER_COPY_STRIDE];
            heapMemOffset_t sizeToCopy = size - offset;
            if (sizeToCopy > BUFFER_COPY_STRIDE) {
                sizeToCopy = BUFFER_COPY_STRIDE;
            }
            readArgRange(buffer, source, offset, sizeToCopy);
            writeArgRange(destination, offset, buffer, sizeToCopy);
            checkUnhandledError();
            offset += sizeToCopy;
        }
    }
}

void evaluateBytecodeInstruction() {
    if (currentInstructionFilePos == instructionBodyEndFilePos) {
        returnFromFunc();
        return;
    }
    uint8_t opcode = readInstructionData(uint8_t);
    uint8_t opcodeCategory = opcode >> 4;
    if (opcodeCategory >= getArrayLength(argAmountOffsetArray) - 1) {
        throw(NO_IMPL_ERR_CODE);
    }
    uint8_t opcodeOffset = opcode & 0x0F;
    int8_t offset1 = readFixedArrayElement(argAmountOffsetArray, opcodeCategory);
    int8_t offset2 = readFixedArrayElement(argAmountOffsetArray, opcodeCategory + 1);
    if (opcodeOffset >= offset2 - offset1) {
        throw(NO_IMPL_ERR_CODE);
    }
    int8_t argAmount = readFixedArrayElement(
        argAmountArray,
        offset1 + opcodeOffset
    );
    for (argParseIndex = 0; argParseIndex < argAmount; argParseIndex++) {
        parseInstructionArg(instructionArgArray + argParseIndex);
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
            // fillBuff.
            instructionArg_t *buffer = instructionArgArray;
            int32_t bufferSize = readArgInt(1);
            validateArgBounds(buffer, bufferSize);
            checkUnhandledError();
            int32_t value = readArgInt(2);
            uint8_t valueType = getArgPrefixDataType((instructionArgArray + 2)->prefix);
            int8_t valueSize = getArgDataTypeSize(valueType);
            if (bufferSize % valueSize != 0) {
                throw(LEN_ERR_CODE);
            }
            for (heapMemOffset_t offset = 0; offset < bufferSize; offset += valueSize) {
                writeArgRange(buffer, offset, (int8_t *)&value, valueSize);
                checkUnhandledError();
            }
        } else if (opcodeOffset == 0x3) {
            // newArgFrame.
            heapMemOffset_t argFrameSize = (heapMemOffset_t)readArgInt(0);
            if (argFrameSize < 0) {
                throw(LEN_ERR_CODE);
            }
            createNextArgFrame(argFrameSize);
        } else if (opcodeOffset == 0x4) {
            // newAlloc.
            int8_t attributes = (int8_t)readArgInt(1) & ALLOC_ATTR_MASK;
            heapMemOffset_t size = (heapMemOffset_t)readArgInt(2);
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
        } else if (opcodeOffset == 0x5) {
            // delAlloc.
            allocPointer_t alloc = readArgDynamicAlloc(0);
            if (!currentImplementerMayAccessAlloc(alloc)) {
                throw(PERM_ERR_CODE);
            }
            deleteAlloc(alloc);
        } else if (opcodeOffset == 0x6) {
            // allocAttrs.
            allocPointer_t alloc = readArgDynamicAlloc(1);
            int8_t attributes = getDynamicAllocMember(alloc, attributes);
            writeArgInt(0, attributes);
        } else if (opcodeOffset == 0x7) {
            // allocSize.
            allocPointer_t alloc = readArgDynamicAlloc(1);
            heapMemOffset_t size = getDynamicAllocSize(alloc);
            writeArgInt(0, size);
        } else if (opcodeOffset == 0x8) {
            // allocCreator.
            allocPointer_t alloc = readArgDynamicAlloc(1);
            allocPointer_t creator = getDynamicAllocMember(alloc, creator);
            if (creator == NULL_ALLOC_POINTER) {
                writeArgInt(0, NULL_ALLOC_POINTER);
            } else {
                allocPointer_t fileHandle = getRunningAppMember(creator, fileHandle);
                writeArgInt(0, fileHandle);
            }
        } else if (opcodeOffset == 0x9) {
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
        int32_t instructionOffset = readArgConstantInt(0);
        int8_t condition = 0;
        if (opcodeOffset == 0x0) {
            // jmp.
            condition = true;
        } else {
            int32_t operand1 = readArgInt(1);
            if (opcodeOffset == 0x1) {
                // jmpZ.
                condition = (operand1 == 0);
            } else if (opcodeOffset == 0x2) {
                // jmpNZ.
                condition = (operand1 != 0);
            } else {
                int32_t operand2 = readArgInt(2);
                if (opcodeOffset == 0x3) {
                    // jmpE.
                    condition = (operand1 == operand2);
                } else if (opcodeOffset == 0x4) {
                    // jmpNE.
                    condition = (operand1 != operand2);
                } else if (opcodeOffset == 0x5) {
                    // jmpG.
                    condition = (operand1 > operand2);
                } else if (opcodeOffset == 0x6) {
                    // jmpNG.
                    condition = (operand1 <= operand2);
                }
            }
        }
        if (condition) {
            jumpToBytecodeInstruction(instructionOffset);
        }
    } else if (opcodeCategory == 0x2) {
        // Gate instructions.
        if (opcodeOffset == 0x0) {
            // newGate.
            int32_t mode = readArgInt(1);
            if (mode < STAY_OPEN_GATE_MODE || mode > PASS_CLOSE_GATE_MODE) {
                throw(TYPE_ERR_CODE);
            }
            allocPointer_t gate = createGate(mode);
            checkUnhandledError();
            writeArgInt(0, gate);
        } else {
            allocPointer_t gate = readArgGate(0);
            if (opcodeOffset == 0x1) {
                // delGate.
                deleteGate(gate);
            } else if (opcodeOffset == 0x2) {
                // waitGate.
                waitForGate(gate);
            } else if (opcodeOffset == 0x3) {
                // openGate.
                openGate(gate);
            } else if (opcodeOffset == 0x4) {
                // closeGate.
                closeGate(gate);
            }
        }
    } else if (opcodeCategory == 0x3) {
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
    } else if (opcodeCategory == 0x4) {
        // Function instructions.
        if (opcodeOffset == 0x0) {
            // findFunc.
            allocPointer_t runningApp = readArgRunningApp(1);
            int32_t funcId = readArgInt(2);
            int32_t funcIndex = findFuncById(runningApp, funcId);
            writeArgInt(0, funcIndex);
        } else if (opcodeOffset == 0x1) {
            // call.
            int32_t funcIndex = readArgInt(0);
            callFunc(currentThread, currentImplementer, funcIndex, false);
        } else if (opcodeOffset == 0x2) {
            // callRemote.
            allocPointer_t implementer = readArgRunningApp(0);
            int32_t funcIndex = readArgInt(1);
            callFunc(currentThread, implementer, funcIndex, true);
        } else if (opcodeOffset == 0x3) {
            // ret.
            returnFromFunc();
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
            int32_t funcIndex = readArgInt(2);
            int8_t isGuarded = funcIsGuarded(implementer, funcIndex);
            checkUnhandledError();
            writeArgInt(0, isGuarded);
        }
    } else if (opcodeCategory == 0x5) {
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
    } else if (opcodeCategory == 0x6) {
        // Arithmetic instructions.
        int32_t result = 0;
        if (opcodeOffset == 0x0) {
            result = readArgInt(0) + 1;
        } else if (opcodeOffset == 0x1) {
            result = readArgInt(0) - 1;
        } else {
            int32_t operand1 = readArgInt(1);
            int32_t operand2 = readArgInt(2);
            if (opcodeOffset == 0x2) {
                // add.
                result = operand1 + operand2;
            } else if (opcodeOffset == 0x3) {
                // sub.
                result = operand1 - operand2;
            } else if (opcodeOffset == 0x4) {
                // mul.
                result = operand1 * operand2;
            } else if (opcodeOffset == 0x5) {
                // div.
                if (operand2 == 0) {
                    throw(NUM_RANGE_ERR_CODE);
                }
                result = operand1 / operand2;
            } else if (opcodeOffset == 0x6) {
                // mod.
                if (operand2 == 0) {
                    throw(NUM_RANGE_ERR_CODE);
                }
                result = operand1 % operand2;
            }
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
            shouldStopCoalescence = true;
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
            heapMemOffset_t offset = 0;
            while (offset < size) {
                int8_t buffer[BUFFER_COPY_STRIDE];
                heapMemOffset_t sizeToCopy = size - offset;
                if (sizeToCopy > BUFFER_COPY_STRIDE) {
                    sizeToCopy = BUFFER_COPY_STRIDE;
                }
                readStorageRange(buffer, contentAddress + offset, sizeToCopy);
                writeArgRange(destination, offset, buffer, sizeToCopy);
                checkUnhandledError();
                offset += sizeToCopy;
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
            heapMemOffset_t offset = 0;
            while (offset < size) {
                int8_t buffer[BUFFER_COPY_STRIDE];
                heapMemOffset_t sizeToCopy = size - offset;
                if (sizeToCopy > BUFFER_COPY_STRIDE) {
                    sizeToCopy = BUFFER_COPY_STRIDE;
                }
                readArgRange(buffer, source, offset, sizeToCopy);
                writeStorageRange(contentAddress + offset, buffer, sizeToCopy);
                offset += sizeToCopy;
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
            heapMemOffset_t nameAddress = getDynamicAllocDataAddress(fileName);
            heapMemOffset_t nameSize = getDynamicAllocSize(fileName);
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
            copyStorageNameToHeapMem(
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
            writeArgInt(0, HEAP_MEM_SIZE);
        } else if (opcodeOffset == 0x1) {
            // appMemSize.
            memUsageContext_t context = {readArgRunningApp(1), 0};
            iterateOverAllocs(&context, (allocIterationHandle_t)registerRunningAppMemUsage);
            writeArgInt(0, context.memUsage);
        } else if (opcodeOffset == 0x2) {
            // memSizeLeft.
            writeArgInt(0, heapMemSizeLeft);
        } else if (opcodeOffset == 0x3) {
            // volSize.
            writeArgInt(0, STORAGE_SIZE);
        } else if (opcodeOffset == 0x4) {
            // volSizeLeft.
            storageOffset_t storageUsage = sizeof(storageHeader_t);
            storageOffset_t address = getFirstFileAddress();
            while (address != MISSING_FILE_ADDRESS) {
                storageUsage += getFileStorageSize(address);
                address = getFileHeaderMember(address, next);
            }
            writeArgInt(0, STORAGE_SIZE - storageUsage);
        }
    }
}

void setTermObserver() {
    allocPointer_t caller = getCurrentCaller();
    int32_t termInputIndex = findFuncById(caller, TERM_INPUT_FUNC_ID);
    if (termInputIndex < 0) {
        throwInSystemApp(MISSING_ERR_CODE);
    }
    allocPointer_t observer = readTermAppGlobalVar(observer);
    if (observer != NULL_ALLOC_POINTER) {
        closeFile(observer);
    }
    observer = getRunningAppMember(caller, fileHandle);
    incrementFileOpenDepth(observer);
    writeTermAppGlobalVar(observer, observer);
    writeTermAppGlobalVar(termInputIndex, termInputIndex);
    returnFromFunc();
}

void killTermApp() {
    allocPointer_t observer = readTermAppGlobalVar(observer);
    if (observer != NULL_ALLOC_POINTER) {
        closeFile(observer);
    }
    hardKillApp(currentImplementer, NONE_ERR_CODE);
}

void resetSystemState() {
    firstFileHandle = NULL_ALLOC_POINTER;
    heapMemSizeLeft = HEAP_MEM_SIZE;
    killStatesDelay = 0;
    unhandledErrorCode = NONE_ERR_CODE;
    for (int8_t degree = 0; degree < SPAN_DEGREE_AMOUNT; degree++) {
        emptySpansByDegree[degree] = MISSING_SPAN_ADDRESS;
    }
    for (int8_t index = 0; index < SPAN_BIT_FIELD_SIZE; index++) {
        emptySpanBitField[index] = 0;
    }
    heapMemOffset_t spanAddress = 0;
    heapMemOffset_t spanSize = HEAP_MEM_SIZE - sizeof(spanHeader_t);
    setSpanMember(spanAddress, previousByNeighbor, MISSING_SPAN_ADDRESS);
    setSpanMember(spanAddress, nextByNeighbor, MISSING_SPAN_ADDRESS);
    setSpanMember(spanAddress, size, spanSize);
    setSpanMember(spanAddress, allocType, NONE_ALLOC_TYPE);
    initializeEmptySpan(spanAddress, spanSize);
    for (int8_t type = 0; type < ALLOC_TYPE_AMOUNT; type++) {
        allocsByType[type] = NULL_ALLOC_POINTER;
    }
    for (int16_t index = 0; index < ALLOC_BIT_FIELD_SIZE; index++) {
        allocBitField[index] = 0;
    }
}


