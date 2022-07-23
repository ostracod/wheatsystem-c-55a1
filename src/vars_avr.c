
#include "./headers.h"

int8_t currentSpiMode = NONE_SPI_MODE;
heapMemOffset_t sramAddress;
storageOffset_t eepromAddress;

int8_t lastPressedButton = 0;
int8_t buttonMode = ACTIONS_BUTTON_MODE;
int8_t firstButtonInPair = 0;

volatile int8_t keyCodeBuffer[KEY_CODE_BUFFER_SIZE];
volatile int8_t keyCodeIndex = 0;
int8_t lastKeyCodeIndex = 0;


