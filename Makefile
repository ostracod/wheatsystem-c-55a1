
AVR_MCU := atmega328p
AVR_CC := avr-gcc
UNIX_CC := gcc
SRC_DIR := src
BUILD_DIR := build
ENSURE_BUILD_DIR := mkdir -p $(BUILD_DIR)
COMMON_SOURCES := $(wildcard $(SRC_DIR)/*_common.c)
AVR_SOURCES := $(COMMON_SOURCES) $(wildcard $(SRC_DIR)/*_avr.c)
UNIX_SOURCES := $(COMMON_SOURCES) $(wildcard $(SRC_DIR)/*_unix.c)
AVR_OBJECTS = $(AVR_SOURCES:.c=.o_avr)
UNIX_OBJECTS := $(UNIX_SOURCES:.c=.o_unix)
AVR_HEX := $(BUILD_DIR)/main_avr.hex
AVR_ELF := $(BUILD_DIR)/main_avr.elf
UNIX_EXECUTABLE := $(BUILD_DIR)/main_unix

avr: $(AVR_HEX) $(AVR_ELF)
	avr-size --format=avr --mcu=$(AVR_MCU) $(AVR_ELF)

unix: $(UNIX_EXECUTABLE)

flash: $(AVR_HEX)
	avrdude -c usbtiny -p atmega328p -B 2 -U flash:w:$(AVR_HEX):i

$(AVR_HEX): $(AVR_ELF)
	avr-objcopy -j .text -j .data -O ihex $^ $@

$(AVR_ELF): $(AVR_OBJECTS)
	$(ENSURE_BUILD_DIR)
	$(AVR_CC) -mmcu=$(AVR_MCU) $^ -o $@

$(UNIX_EXECUTABLE): $(UNIX_OBJECTS)
	$(ENSURE_BUILD_DIR)
	$(UNIX_CC) -lncurses $^ -o $@

%.o_avr: %.c
	$(AVR_CC) -Wno-char-subscripts -Os -D WHEATSYSTEM_AVR -DF_CPU=8000000 -mmcu=$(AVR_MCU) -fstack-usage -c $^ -o $@

%.o_unix: %.c
	$(UNIX_CC) -Wall -c -D WHEATSYSTEM_UNIX -D _GNU_SOURCE $^ -o $@

clean:
	rm -f $(wildcard $(SRC_DIR)/*.o_avr) $(wildcard $(SRC_DIR)/*.o_unix) $(wildcard $(SRC_DIR)/*.su) $(AVR_ELF) $(AVR_HEX) $(UNIX_EXECUTABLE)


