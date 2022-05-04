
#include "./headers.h"
#include <avr/io.h>
#include <util/delay.h>

void initializeSpi() {
    
    DDRB |= 1 << DDB5; // SCK.
    DDRB &= ~(1 << DDB4); // MISO.
    DDRB |= 1 << DDB3; // MOSI.
    
    DDRB |= 1 << DDB2; // Fix SS pin.
    SPCR = (1 << SPE) | (1 << MSTR);
}

void acquireSpiDevice(int8_t id) {
    if (currentSpiDeviceId == id) {
        return;
    }
    releaseEepromSpiDevice();
    releaseLcdSpiDevice();
    currentSpiDeviceId = id;
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

int8_t initializeStorageSpace() {
    eepromCsPinHigh();
    eepromCsPinOutput();
    return true;
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
    // TODO: Implement.
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


