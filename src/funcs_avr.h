
#include <util/delay.h>

// Retrieves a value within the given fixed array.
// "name" is a fixed array.
// "index" is the offset of byte from which to start reading.
#define readFixedArrayValue(name, index, type) ({int8_t result[sizeof(type)]; memcpy_P(&result, (int8_t *)name + index, sizeof(type)); *(type *)result;})

#define eepromCsPinOutput() DDRD |= 1 << DDD3
#define eepromCsPinHigh() PORTD |= 1 << PORTD3
#define eepromCsPinLow() PORTD &= ~(1 << PORTD3)

// Persists any pending changes to non-volatile storage.
// Since writeStorageSpaceRange writes to EEPROM immediately, flushStorageSpace does not need to do anything.
#define flushStorageSpace()

#define sleepMilliseconds(milliseconds) _delay_ms(milliseconds)

// Sets up the AVR SPI bus. Must be called before using the SPI bus.
void initializeSpi();
// Changes the currently active SPI device. Raises or lowers chip select pins as necessary.
void acquireSpiDevice(int8_t id);
// Blocks execution until the SPI bus receives one byte. Returns the byte received over the SPI bus.
int8_t receiveSpiInt8();
// Sends the given value over the SPI bus.
void sendSpiInt8(int8_t value);

// Finishes access to EEPROM over SPI.
void releaseEepromSpiDevice();
// Sends an address to EEPROM over SPI.
void sendAddressToEeprom();
// Reads an interval of data from non-volatile storage.
// "address" is the offset of first byte to read.
void readStorageSpaceRange(void *destination, int32_t address, int32_t amount);
// Writes an interval of data to non-volatile storage. Changes might not be persisted until calling flushStorageSpace.
// "address" is the offset of first byte to write.
void writeStorageSpaceRange(int32_t address, void *source, int32_t amount);

// Finishes access to LCD over SPI.
void releaseLcdSpiDevice();


