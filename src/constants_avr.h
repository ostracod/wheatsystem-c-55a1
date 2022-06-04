
#define NULL ((void *)0)

#define SRAM_SPI_DEVICE_ID 1
#define EEPROM_SPI_DEVICE_ID 2
#define LCD_SPI_DEVICE_ID 3

#define BAUD_RATE 9600
#define BAUD_RATE_NUMBER (F_CPU / (16 * (int32_t)BAUD_RATE) - 1)

// Fixed array of values.
const int8_t lcdInitCommands[9];


