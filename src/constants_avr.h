
#define NULL ((void *)0)

#define NONE_SPI_DEVICE 0x00
#define SRAM_SPI_DEVICE 0x10
#define EEPROM_SPI_DEVICE 0x20
#define LCD_SPI_DEVICE 0x30

#define NONE_SPI_ACTION 0x00
#define COMMAND_SPI_ACTION 0x01
#define READ_SPI_ACTION 0x02
#define WRITE_SPI_ACTION 0x03

#define NONE_SPI_MODE (NONE_SPI_DEVICE | NONE_SPI_ACTION)
#define EEPROM_WRITE_SPI_MODE (EEPROM_SPI_DEVICE | WRITE_SPI_ACTION)

#define BAUD_RATE 9600
#define BAUD_RATE_NUMBER (F_CPU / (16 * (int32_t)BAUD_RATE) - 1)

// Fixed array of values.
const int8_t lcdInitCommands[9];


