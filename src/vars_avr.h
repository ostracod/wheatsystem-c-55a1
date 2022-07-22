
int8_t currentSpiMode;
heapMemOffset_t sramAddress;
storageOffset_t eepromAddress;

int8_t lastPressedButton;
int8_t buttonMode;
int8_t firstButtonInPair;

volatile int8_t keyCodeBuffer[KEY_CODE_BUFFER_SIZE];
volatile int8_t keyCodeIndex;


