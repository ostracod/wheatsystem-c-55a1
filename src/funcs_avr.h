
#include <util/delay.h>

// Retrieves a value within the given fixed array.
// "name" is a fixed array.
// "index" is the offset of byte from which to start reading.
#define readFixedArrayValue(name, index, type) ({int8_t result[sizeof(type)]; memcpy_P(&result, (int8_t *)name + index, sizeof(type)); *(type *)result;})

#define sleepMilliseconds(milliseconds) _delay_ms(milliseconds)

#define sramCsPinOutput() DDRD |= 1 << DDD4
#define sramCsPinHigh() PORTD |= 1 << PORTD4
#define sramCsPinLow() PORTD &= ~(1 << PORTD4)

#define eepromCsPinOutput() DDRD |= 1 << DDD3
#define eepromCsPinHigh() PORTD |= 1 << PORTD3
#define eepromCsPinLow() PORTD &= ~(1 << PORTD3)

#define lcdCsPinOutput() DDRC |= 1 << DDC3
#define lcdCsPinHigh() PORTC |= 1 << PORTC3
#define lcdCsPinLow() PORTC &= ~(1 << PORTC3)

#define lcdResetPinOutput() DDRC |= (1 << DDC5)
#define lcdResetPinHigh() PORTC |= (1 << PORTC5)
#define lcdResetPinLow() PORTC &= ~(1 << PORTC5)

#define lcdModePinOutput() DDRC |= (1 << DDC4)
#define lcdModePinHigh() PORTC |= (1 << PORTC4)
#define lcdModePinLow() PORTC &= ~(1 << PORTC4)

#define buttonRow1PinOutput() DDRB |= (1 << DDB1)
#define buttonRow1PinInput() DDRB &= ~(1 << DDB1)
#define buttonRow1PinLow() PORTB &= ~(1 << PORTB1)

#define buttonRow2PinOutput() DDRD |= (1 << DDD7)
#define buttonRow2PinInput() DDRD &= ~(1 << DDD7)
#define buttonRow2PinLow() PORTD &= ~(1 << PORTD7)

#define buttonRow3PinOutput() DDRB |= (1 << DDB0)
#define buttonRow3PinInput() DDRB &= ~(1 << DDB0)
#define buttonRow3PinLow() PORTB &= ~(1 << PORTB0)

#define buttonColumn1PinInput() DDRC &= ~(1 << DDC0)
#define buttonColumn1PinRead() (PINC & (1 << PINC0))

#define buttonColumn2PinInput() DDRC &= ~(1 << DDC1)
#define buttonColumn2PinRead() (PINC & (1 << PINC1))

#define buttonColumn3PinInput() DDRD &= ~(1 << DDD2)
#define buttonColumn3PinRead() (PIND & (1 << PIND2))

#define buttonColumn4PinInput() DDRC &= ~(1 << DDC2)
#define buttonColumn4PinRead() (PINC & (1 << PINC2))

// Sets up the AVR SPI bus. Must be called before using the SPI bus.
#define initializeSpi() SPCR = (1 << SPE) | (1 << MSTR)
// Blocks execution until the SPI bus receives one byte. Returns the byte received over the SPI bus.
#define receiveSpiInt8() ({ \
    SPDR = 0xFF; \
    while (!(SPSR & (1 << SPIF))) {} \
    SPDR; \
})
// Sends the given value over the SPI bus.
#define sendSpiInt8(value) \
    SPDR = value; \
    while (!(SPSR & (1 << SPIF))) {}

// Sends an address to SRAM over SPI.
#define sendAddressToSram(inputAddress) { \
    int16_t address = inputAddress; \
    sendSpiInt8(*((int8_t *)&address + 1)); \
    sendSpiInt8(*(int8_t *)&address); \
}
// Reads a value from heap memory.
// "address" is the address of first byte to read.
#define readHeapMem(address, type) \
    ({type result; readHeapMemRange(&result, address, sizeof(type)); result;})
// Writes a value to heap memory.
// "address" is the address of first byte to write.
#define writeHeapMem(address, type, value) \
    ({type tempValue = value; writeHeapMemRange(address, &tempValue, sizeof(type));})

// Sends an address to EEPROM over SPI.
#define sendAddressToEeprom(inputAddress) { \
    int32_t address = inputAddress; \
    sendSpiInt8(*((int8_t *)&address + 2)); \
    sendSpiInt8(*((int8_t *)&address + 1)); \
    sendSpiInt8(*(int8_t *)&address); \
}
// External EEPROM does not require any initialization beyond setting pin modes.
#define initializeStorage() true

// Determines the next location where characters will be added to the LCD.
#define setLcdCursorPos(posX, posY) sendLcdCommand(0x80 | (posX + posY * 0x40))

// Retrieves a global variable of the serial driver.
// "memberName" is the name of a member in serialAppGlobalFrame_t.
#define readSerialAppGlobalVar(memberName) \
    readSystemAppGlobalVar(serialAppGlobalFrame_t, memberName)
// Modifies a global variable of the serial driver.
// "memberName" is the name of a member in serialAppGlobalFrame_t.
#define writeSerialAppGlobalVar(memberName, value) \
    writeSystemAppGlobalVar(serialAppGlobalFrame_t, memberName, value)

#define sleepAfterSchedule()

// We do not support integration tests on the AVR platform yet.
#define handleTestInstruction()
#define resetHaltFlag()
#define checkHaltFlag()

// Sets the initial mode of each AVR GPIO pin.
void initializePinModes();

// Changes the currently active SPI mode. Mode number = device ID | action ID.
void setSpiMode(int8_t mode);

// Sets up serial UART communication.
void initializeUart();
int8_t receiveUartInt8Helper();
// Blocks execution until UART receives one byte. Returns the byte received over UART.
int8_t receiveUartInt8();
// Sends a byte over UART.
void sendUartInt8(int8_t character);

// Sets up external SRAM over SPI.
void initializeSram();
// Reads an interval of data from heap memory.
// "address" is the offset of first byte to read.
void readHeapMemRange(
    void *destination,
    heapMemOffset_t address,
    heapMemOffset_t amount
);
// Writes an interval of data to heap memory.
// "address" is the offset of first byte to write.
void writeHeapMemRange(
    heapMemOffset_t address,
    void *source,
    heapMemOffset_t amount
);

// Reads an interval of data from non-volatile storage.
// "address" is the offset of first byte to read.
void readStorageRange(
    void *destination,
    storageOffset_t address,
    storageOffset_t amount
);
// Writes an interval of data to non-volatile storage. Changes might not be persisted until calling flushStorage.
// "address" is the offset of first byte to write.
void writeStorageRange(storageOffset_t address, void *source, storageOffset_t amount);
// Persists any pending changes to non-volatile storage.
void flushStorage();

// Sends a command to the character LCD.
void sendLcdCommand(int8_t command);
// Sends a character to display on the LCD.
void sendLcdCharacter(int8_t character);
// Sets up the character LCD.
void initializeLcd();

// Determines which column is pressed in the given row of buttons.
// "row" is a number between 0 and 2 inclusive.
// Returns 1 through 4 if a single column is pressed, 0 if no column is pressed, or -1 if multiple columns are pressed.
int8_t getPressedButtonColumn(int8_t row);
// Determines which button is pressed on the keypad.
// Returns 1 through 12 if a single button is pressed, 0 if no button is pressed, or -1 if multiple buttons are pressed.
// The button numbers are ordered on the keypad like so:
// +----+----+----+----+
// | 1  | 2  | 3  | 4  |
// +----+----+----+----+
// | 5  | 6  | 7  | 8  |
// +----+----+----+----+
// | 9  | 10 | 11 | 12 |
// +----+----+----+----+
int8_t getPressedButton();

int8_t getKeyCodeOffset(int8_t button);
void registerKeyCode(int8_t keyCode);
void registerPressedButton(int8_t button);
void initializeButtonTimer();

// Runs a mode where EEPROM data may be written and read over UART.
void runTransferMode();

// Implements the "init" WheatSystem function.
void initializeSerialApp();
// Implements the "startSerial" WheatSystem function.
void startSerialApp();
// Implements the "stopSerial" WheatSystem function.
void stopSerialApp();
// Implements the "wrtSerial" WheatSystem function.
void writeSerialApp();
// Implements the "kill" WheatSystem function.
void killSerialApp();


