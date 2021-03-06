
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
#define SRAM_READ_SPI_MODE (SRAM_SPI_DEVICE | READ_SPI_ACTION)
#define SRAM_WRITE_SPI_MODE (SRAM_SPI_DEVICE | WRITE_SPI_ACTION)
#define EEPROM_READ_SPI_MODE (EEPROM_SPI_DEVICE | READ_SPI_ACTION)
#define EEPROM_WRITE_SPI_MODE (EEPROM_SPI_DEVICE | WRITE_SPI_ACTION)

#define LCD_WIDTH 16
#define LCD_HEIGHT 2

#define CHARS_BUTTON 4
#define ACTIONS_BUTTON 8

#define CHARS_BUTTON_MODE 1
#define ACTIONS_BUTTON_MODE 2

#define KEY_CODE_BUFFER_SIZE 5
#define UART_BUFFER_SIZE 64
#define TRANSFER_MODE_BUFFER_SIZE 128

// Global frame size of the serial driver.
#define SERIAL_APP_GLOBAL_FRAME_SIZE sizeof(serialAppGlobalFrame_t)

// Fixed array of values.
const int8_t lcdInitCommands[9];

// Fixed array of characters.
const int8_t transferModeStringConstant[25];

// Fixed arrays of keycodes.
const int8_t charsModeKeyCodes[100];
const int8_t actionsModeKeyCodes[10];

// List of functions which the serial driver implements. Fixed array of values.
const systemAppFunc_t serialAppFuncArray[5];

// Determines the system apps which are available on the AVR platform. Fixed array of values.
const systemApp_t systemAppArray[2];


