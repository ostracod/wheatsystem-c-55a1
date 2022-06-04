
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

void acquireSpiDevice(int8_t id) {
    if (currentSpiDeviceId == id) {
        return;
    }
    releaseSramSpiDevice();
    releaseEepromSpiDevice();
    releaseLcdSpiDevice();
    currentSpiDeviceId = id;
    _delay_us(5);
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

void releaseSramSpiDevice() {
    sramCsPinHigh();
}

void initializeSram() {
    // Enable SRAM sequential mode.
    acquireSpiDevice(SRAM_SPI_DEVICE_ID);
    sramCsPinLow();
    sendSpiInt8(0x01);
    sendSpiInt8(0x81);
    sramCsPinHigh();
    _delay_us(5);
}

void releaseEepromSpiDevice() {
    eepromCsPinHigh();
    eepromAddress = -100;
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
    acquireSpiDevice(EEPROM_SPI_DEVICE_ID);
    int32_t index = 0;
    if (address == eepromAddress - 1) {
        *(int8_t *)(destination + index) = lastEepromData;
        index += 1;
    } else if (address != eepromAddress) {
        eepromCsPinHigh();
        _delay_us(5);
        eepromCsPinLow();
        sendSpiInt8(0x03);
        sendAddressToEeprom(address);
        eepromAddress = address;
    }
    while (index < amount) {
        int8_t tempData = receiveSpiInt8();
        *(int8_t *)(destination + index) = tempData;
        lastEepromData = tempData;
        eepromAddress += 1;
        index += 1;
    }
}

void writeStorageSpaceRange(int32_t address, void *source, int32_t amount) {
    if (amount <= 0) {
        return;
    }
    acquireSpiDevice(EEPROM_SPI_DEVICE_ID);
    eepromAddress = address;
    int8_t tempShouldWrite = true;
    int32_t index = 0;
    while (tempShouldWrite) {
        eepromCsPinHigh();
        _delay_us(5);
        eepromCsPinLow();
        sendSpiInt8(0x06);
        eepromCsPinHigh();
        _delay_us(5);
        eepromCsPinLow();
        sendSpiInt8(0x02);
        sendAddressToEeprom(eepromAddress);
        while (true) {
            if (index >= amount) {
                tempShouldWrite = false;
                break;
            }
            sendSpiInt8(*(int8_t *)(source + index));
            eepromAddress += 1;
            index += 1;
            if (eepromAddress % 256 == 0) {
                break;
            }
        }
        eepromCsPinHigh();
        _delay_ms(8);
    }
    eepromAddress = -100;
}

void releaseLcdSpiDevice() {
    lcdCsPinHigh();
}

void sendLcdCommand(int8_t command) {
    acquireSpiDevice(LCD_SPI_DEVICE_ID);
    lcdModePinLow();
    lcdCsPinLow();
    sendSpiInt8(command);
    sleepMilliseconds(5);
}

void sendLcdCharacter(int8_t character) {
    acquireSpiDevice(LCD_SPI_DEVICE_ID);
    lcdModePinHigh();
    lcdCsPinLow();
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
    // TODO: Implement.
}

void setTermObserver() {
    // TODO: Implement.
}

void getTermSize() {
    // TODO: Implement.
}

void writeTermText() {
    // TODO: Implement.
}


