
#include "./headers.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void initializePinModes() {
    
    DDRB |= (1 << DDB5); // SCK.
    DDRB &= ~(1 << DDB4); // MISO.
    DDRB |= (1 << DDB3); // MOSI.
    DDRB |= (1 << DDB2); // Fix SS pin.
    
    sramCsPinHigh();
    sramCsPinOutput();
    
    eepromCsPinHigh();
    eepromCsPinOutput();
    
    lcdCsPinHigh();
    lcdResetPinHigh();
    lcdCsPinOutput();
    lcdModePinOutput();
    lcdResetPinOutput();
    
    buttonRow1PinLow();
    buttonRow2PinLow();
    buttonRow3PinLow();
    buttonRow1PinInput();
    buttonRow2PinInput();
    buttonRow3PinInput();
    
    buttonColumn1PinInput();
    buttonColumn2PinInput();
    buttonColumn3PinInput();
    buttonColumn4PinInput();
}

void setSpiMode(int8_t mode) {
    if (currentSpiMode == mode) {
        return;
    }
    sramCsPinHigh();
    eepromCsPinHigh();
    lcdCsPinHigh();
    if (currentSpiMode == EEPROM_WRITE_SPI_MODE) {
        _delay_ms(8);
    } else {
        _delay_us(5);
    }
    currentSpiMode = mode;
    int8_t spiDevice = currentSpiMode & 0xF0;
    if (spiDevice == SRAM_SPI_DEVICE) {
        sramCsPinLow();
    } else if (spiDevice == EEPROM_SPI_DEVICE) {
        eepromCsPinLow();
    } else if (spiDevice == LCD_SPI_DEVICE) {
        lcdCsPinLow();
    }
}

int8_t receiveSpiInt8() {
    SPDR = 0xFF;
    while (!(SPSR & (1 << SPIF))) {
        
    }
    return SPDR;
}

void sendSpiInt8(int8_t value) {
    SPDR = value;
    while (!(SPSR & (1 << SPIF))) {
        
    }
}

void initializeUart() {
    // Set UART baud rate.
    UBRR0L = BAUD_RATE_NUMBER & 0xFF;
    UBRR0H = BAUD_RATE_NUMBER >> 8;
    // Enable UART transmitter and receiver.
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);
}

int8_t receiveUartInt8() {
    // Wait for receive buffer to be full.
    while (!(UCSR0A & (1 << RXC0))) {
        
    }
    // Read character from receive register.
    return UDR0;
}

void sendUartInt8(int8_t character) {
    // Wait for transmit buffer to be empty.
    while (!(UCSR0A & (1 << UDRE0))) {
        
    }
    // Load character into transmit register.
    UDR0 = character;
}

void initializeSram() {
    // Enable SRAM sequential mode.
    setSpiMode(SRAM_SPI_DEVICE | COMMAND_SPI_ACTION);
    sendSpiInt8(0x01);
    sendSpiInt8(0x41);
    setSpiMode(NONE_SPI_MODE);
}

void sendAddressToSram(int16_t address) {
    sendSpiInt8((address & 0xFF00) >> 8);
    sendSpiInt8(address & 0x00FF);
}

void readHeapMemRange(
    void *destination,
    heapMemOffset_t address,
    heapMemOffset_t amount
) {
    int8_t shouldStartRead = true;
    if (currentSpiMode == SRAM_READ_SPI_MODE) {
        if (sramAddress == address) {
            shouldStartRead = false;
        } else {
            setSpiMode(NONE_SPI_MODE);
        }
    }
    if (shouldStartRead) {
        setSpiMode(SRAM_READ_SPI_MODE);
        sramAddress = address;
        sendSpiInt8(0x03);
        sendAddressToSram(sramAddress);
    }
    for (heapMemOffset_t index = 0; index < amount; index++) {
        int8_t value = receiveSpiInt8();
        *(int8_t *)(destination + index) = value;
    }
    sramAddress += amount;
}

void writeHeapMemRange(
    heapMemOffset_t address,
    void *source,
    heapMemOffset_t amount
) {
    int8_t shouldStartWrite = true;
    if (currentSpiMode == SRAM_WRITE_SPI_MODE) {
        if (sramAddress == address) {
            shouldStartWrite = false;
        } else {
            setSpiMode(NONE_SPI_MODE);
        }
    }
    if (shouldStartWrite) {
        setSpiMode(SRAM_WRITE_SPI_MODE);
        sramAddress = address;
        sendSpiInt8(0x02);
        sendAddressToSram(sramAddress);
    }
    for (heapMemOffset_t index = 0; index < amount; index++) {
        int8_t value = *(int8_t *)(source + index);
        sendSpiInt8(value);
    }
    sramAddress += amount;
}

void sendAddressToEeprom(storageOffset_t address) {
    sendSpiInt8((address & 0x00FF0000) >> 16);
    sendSpiInt8((address & 0x0000FF00) >> 8);
    sendSpiInt8(address & 0x000000FF);
}

void readStorageRange(
    void *destination,
    storageOffset_t address,
    storageOffset_t amount
) {
    if (amount <= 0) {
        return;
    }
    int8_t shouldStartRead = true;
    if (currentSpiMode == EEPROM_READ_SPI_MODE) {
        if (eepromAddress == address) {
            shouldStartRead = false;
        } else {
            setSpiMode(NONE_SPI_MODE);
        }
    }
    if (shouldStartRead) {
        setSpiMode(EEPROM_READ_SPI_MODE);
        eepromAddress = address;
        sendSpiInt8(0x03);
        sendAddressToEeprom(eepromAddress);
    }
    for (storageOffset_t index = 0; index < amount; index++) {
        int8_t value = receiveSpiInt8();
        *(int8_t *)(destination + index) = value;
    }
    eepromAddress += amount;
}

void writeStorageRange(storageOffset_t address, void *source, storageOffset_t amount) {
    if (amount <= 0) {
        return;
    }
    storageOffset_t targetAddress = address;
    for (storageOffset_t index = 0; index < amount; index++) {
        if (currentSpiMode != EEPROM_WRITE_SPI_MODE || eepromAddress != targetAddress) {
            eepromAddress = targetAddress;
            setSpiMode(EEPROM_SPI_DEVICE | COMMAND_SPI_ACTION);
            sendSpiInt8(0x06);
            setSpiMode(EEPROM_WRITE_SPI_MODE);
            sendSpiInt8(0x02);
            sendAddressToEeprom(eepromAddress);
        }
        int8_t value = *(int8_t *)(source + index);
        sendSpiInt8(value);
        eepromAddress += 1;
        targetAddress += 1;
        if ((eepromAddress & 0x000000FF) == 0) {
            setSpiMode(NONE_SPI_MODE);
        }
    }
}

void flushStorage() {
    if (currentSpiMode == EEPROM_WRITE_SPI_MODE) {
        setSpiMode(NONE_SPI_MODE);
    }
}

void sendLcdCommand(int8_t command) {
    setSpiMode(LCD_SPI_DEVICE | COMMAND_SPI_ACTION);
    lcdModePinLow();
    sendSpiInt8(command);
    sleepMilliseconds(5);
}

void sendLcdCharacter(int8_t character) {
    setSpiMode(LCD_SPI_DEVICE | WRITE_SPI_ACTION);
    lcdModePinHigh();
    sendSpiInt8(character);
    sleepMilliseconds(2);
}

void initializeLcd() {
    
    sleepMilliseconds(20);
    lcdResetPinLow();
    sleepMilliseconds(20);
    lcdResetPinHigh();
    sleepMilliseconds(20);
    
    for (int8_t index = 0; index < sizeof(lcdInitCommands); index++) {
        int8_t command = pgm_read_byte(lcdInitCommands + index);
        sendLcdCommand(command);
    }
}

int8_t getPressedButtonColumn(int8_t row) {
    if (row == 0) {
        buttonRow1PinOutput();
    } else if (row == 1) {
        buttonRow2PinOutput();
    } else if (row == 2) {
        buttonRow3PinOutput();
    }
    // Believe it or not, a delay is required so that the
    // electronic signal can propagate through the buttons.
    _delay_us(10);
    int8_t column = 0;
    int8_t columnCount = 0;
    if (!buttonColumn1PinRead()) {
        column = 1;
        columnCount += 1;
    }
    if (!buttonColumn2PinRead()) {
        column = 2;
        columnCount += 1;
    }
    if (!buttonColumn3PinRead()) {
        column = 3;
        columnCount += 1;
    }
    if (!buttonColumn4PinRead()) {
        column = 4;
        columnCount += 1;
    }
    buttonRow1PinInput();
    buttonRow2PinInput();
    buttonRow3PinInput();
    return (columnCount > 1) ? -1 : column;
}

int8_t getPressedButton() {
    int8_t output = 0;
    for (int8_t row = 0; row < 3; row += 1) {
        int8_t column = getPressedButtonColumn(row);
        if (column < 0) {
            return -1;
        } else if (column > 0) {
            if (output > 0) {
                return -1;
            } else {
                output = column + row * 4;
            }
        }
    }
    return output;
}

int8_t getKeyCodeOffset(int8_t button) {
    if (button < CHARS_BUTTON) {
        return button - 1;
    } else if (button < ACTIONS_BUTTON) {
        return button - 2;
    } else {
        return button - 3;
    }
}

void registerKeyCode(int8_t keyCode) {
    keyCodeIndex += 1;
    if (keyCodeIndex >= KEY_CODE_BUFFER_SIZE) {
        keyCodeIndex = 0;
    }
    keyCodeBuffer[keyCodeIndex] = keyCode;
}

void registerPressedButton(int8_t button) {
    if (button == CHARS_BUTTON) {
        buttonMode = CHARS_BUTTON_MODE;
        firstButtonInPair = 0;
    } else if (button == ACTIONS_BUTTON) {
        buttonMode = ACTIONS_BUTTON_MODE;
    } else if (buttonMode == ACTIONS_BUTTON_MODE) {
        int8_t index = getKeyCodeOffset(button);
        int8_t keyCode = pgm_read_byte(actionsModeKeyCodes + index);
        registerKeyCode(keyCode);
    } else if (firstButtonInPair == 0) {
        firstButtonInPair = button;
    } else {
        int8_t index = getKeyCodeOffset(firstButtonInPair) * 10 + getKeyCodeOffset(button);
        int8_t keyCode = pgm_read_byte(charsModeKeyCodes + index);
        registerKeyCode(keyCode);
        firstButtonInPair = 0;
    }
}

void initializeButtonTimer() {
    // Enable CTC timer mode, and use clock divided by 1024.
    TCCR1B |= (1 << WGM12) | (1 << CS02) | (1 << CS00);
    // Set maximum timer value to be 5 ms.
    OCR1A = 390;
    // Set initial timer value.
    TCNT1 = 0;
    // Configure interrupt to run when timer reaches maximum value.
    TIMSK1 |= (1 << OCIE1A);
    // Enable interrupts.
    sei();
}

// Interrupt triggered by timer.
ISR(TIMER1_COMPA_vect) {
    int8_t button = getPressedButton();
    if (button >= 0 && button != lastPressedButton) {
        if (button > 0) {
            registerPressedButton(button);
        }
        lastPressedButton = button;
    }
}

void runTransferMode() {
    
    sendLcdCommand(0x80);
    int8_t index = 0;
    while (true) {
        int8_t character = pgm_read_byte(transferModeStringConstant + index);
        if (character == 0) {
            break;
        } else if (character == '\n') {
            sendLcdCommand(0xC0);
        } else {
            sendLcdCharacter(character);
        }
        index += 1;
    }
    
    while (true) {
        while (true) {
            int8_t prefix = receiveUartInt8();
            if (prefix == '!') {
                break;
            }
        }
        int8_t command = receiveUartInt8();
        storageOffset_t address = 0;
        ((int8_t *)&address)[0] = receiveUartInt8();
        ((int8_t *)&address)[1] = receiveUartInt8();
        ((int8_t *)&address)[2] = receiveUartInt8();
        int8_t buffer[TRANSFER_MODE_BUFFER_SIZE];
        if (command == 'R') {
            readStorageRange(buffer, address, TRANSFER_MODE_BUFFER_SIZE);
            sendUartInt8('!');
            for (int16_t index = 0; index < TRANSFER_MODE_BUFFER_SIZE; index++) {
                sendUartInt8(buffer[index]);
            }
        } else if (command == 'W') {
            for (int16_t index = 0; index < TRANSFER_MODE_BUFFER_SIZE; index++) {
                buffer[index] = receiveUartInt8();
            }
            writeStorageRange(address, buffer, TRANSFER_MODE_BUFFER_SIZE);
            flushStorage();
            sendUartInt8('!');
        }
    }
}

void initializeTermApp() {
    allocPointer_t observer = readTermAppGlobalVar(observer);
    if (observer == NULL_ALLOC_POINTER) {
        return;
    }
    allocPointer_t runningApp = getFileHandleRunningApp(observer);
    if (runningApp == NULL_ALLOC_POINTER) {
        return;
    }
    int8_t tempIndex = keyCodeIndex;
    if (lastKeyCodeIndex == tempIndex) {
        return;
    }
    lastKeyCodeIndex += 1;
    if (lastKeyCodeIndex >= KEY_CODE_BUFFER_SIZE) {
        lastKeyCodeIndex = 0;
    }
    int8_t key = keyCodeBuffer[lastKeyCodeIndex];
    allocPointer_t nextArgFrame = createNextArgFrame(1);
    checkErrorInSystemApp();
    writeArgFrame(nextArgFrame, 0, int8_t, key);
    int32_t termInputIndex = readTermAppGlobalVar(termInputIndex);
    callFunc(currentThread, runningApp, termInputIndex, true);
    checkErrorInSystemApp();
}

void getTermSize() {
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    writeArgFrame(previousArgFrame, 0, int32_t, LCD_WIDTH);
    writeArgFrame(previousArgFrame, 4, int32_t, LCD_HEIGHT);
    returnFromFunc();
}

void writeTermText() {
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    int32_t posX = readArgFrame(previousArgFrame, 0, int32_t);
    int32_t posY = readArgFrame(previousArgFrame, 4, int32_t);
    allocPointer_t textAlloc = readArgFrame(previousArgFrame, 8, int32_t);
    validateDynamicAlloc(textAlloc);
    checkErrorInSystemApp();
    if (!runningAppMayAccessAlloc(getCurrentCaller(), textAlloc)
            || !currentImplementerMayAccessAlloc(textAlloc)) {
        throwInSystemApp(PERM_ERR_CODE);
    }
    heapMemOffset_t textSize = getDynamicAllocSize(textAlloc);
    sendLcdCommand(0x80 | (posX + posY * 0x40));
    for (heapMemOffset_t index = 0; index < textSize; index++) {
        int8_t character = readDynamicAlloc(textAlloc, index, int8_t);
        sendLcdCharacter(character);
    }
    returnFromFunc();
}


