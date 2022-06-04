
#include "./headers.h"
#include <avr/io.h>
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
}

int8_t setSpiMode(int8_t mode) {
    if (currentSpiMode == mode) {
        return false;
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
    return true;
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

void sendUartCharacter(int8_t character) {
    // Wait for transmit buffer to be empty.
    while (!(UCSR0A & (1 << UDRE0))) {
        
    }
    // Load character into transmit register.
    UDR0 = character;
}

int8_t receiveUartCharacter() {
    // Wait for receive buffer to be full.
    while (!(UCSR0A & (1 << RXC0))) {
        
    }
    // Read character from receive register.
    return UDR0;
}

void initializeSram() {
    // Enable SRAM sequential mode.
    setSpiMode(SRAM_SPI_DEVICE | COMMAND_SPI_ACTION);
    sendSpiInt8(0x01);
    sendSpiInt8(0x81);
    setSpiMode(NONE_SPI_MODE);
}

void sendAddressToEeprom(int32_t address) {
    sendSpiInt8((address & 0x00FF0000) >> 16);
    sendSpiInt8((address & 0x0000FF00) >> 8);
    sendSpiInt8(address & 0x000000FF);
}

void readStorageSpaceRange(void *destination, int32_t address, int32_t amount) {
    if (amount <= 0) {
        return;
    }
    int8_t modeHasChanged = setSpiMode(EEPROM_SPI_DEVICE | READ_SPI_ACTION);
    if (modeHasChanged || eepromAddress != address) {
        eepromAddress = address;
        sendSpiInt8(0x03);
        sendAddressToEeprom(eepromAddress);
    }
    for (int32_t index = 0; index < amount; index++) {
        int8_t value = receiveSpiInt8();
        *(int8_t *)(destination + index) = value;
        eepromAddress += 1;
    }
}

void writeStorageSpaceRange(int32_t address, void *source, int32_t amount) {
    if (amount <= 0) {
        return;
    }
    int32_t targetAddress = address;
    for (int32_t index = 0; index < amount; index++) {
        int8_t modeHasChanged = setSpiMode(EEPROM_WRITE_SPI_MODE);
        if (modeHasChanged || eepromAddress != targetAddress) {
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

void flushStorageSpace() {
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

void initializeTermApp() {
    // Do nothing.
}

void setTermObserver() {
    // TODO: Implement.
}

void getTermSize() {
    // TODO: Implement.
}

void writeTermText() {
    allocPointer_t previousArgFrame = getPreviousArgFrame();
    int32_t posX = readArgFrame(previousArgFrame, 0, int32_t);
    int32_t posY = readArgFrame(previousArgFrame, 4, int32_t);
    allocPointer_t textAlloc = readArgFrame(previousArgFrame, 8, int32_t);
    heapMemoryOffset_t textSize = getDynamicAllocSize(textAlloc);
    sendLcdCommand(0x80 | (posX + posY * 0x40));
    for (heapMemoryOffset_t index = 0; index < textSize; index++) {
        int8_t character = readDynamicAlloc(textAlloc, index, int8_t);
        sendLcdCharacter(character);
    }
    returnFromFunction();
}


