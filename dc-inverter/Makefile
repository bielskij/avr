CURRENT_DIR=$(shell pwd)

ifeq ($(CROSS_COMPILE),)
 CROSS_COMPILE=avr-
endif

ifneq ($(V),1)
 MAKEFLAGS += --silent
endif

MCU_TYPE := attiny13a

F_CPU := 9600000UL

CC       = $(CROSS_COMPILE)gcc
AS       = $(CROSS_COMPILE)as
OBJDUMP  = $(CROSS_COMPILE)objdump
OBJCOPY  = $(CROSS_COMPILE)objcopy
SIZE     = $(CROSS_COMPILE)size
AVRDUDE  = avrdude

DIR_INC := $(CURRENT_DIR)/include
DIR_SRC := $(CURRENT_DIR)/src
DIR_OUT := $(CURRENT_DIR)/out

SRC := $(shell find $(DIR_SRC) -name '*.c' | sort)
TMP := $(foreach file, $(SRC), $(shell echo $(file) | sed -e 's|$(DIR_SRC)\/|$(DIR_OUT)\/|'))

OBJ_SUFFIX := .c.o
OBJS := $(foreach file, $(TMP), $(shell echo $(file) | sed -e 's|\.c$$|$(OBJ_SUFFIX)|'))

INCLUDES = -I$(DIR_INC)

ifndef $(DEBUG)
	DEBUG=0
endif

CFLAGS += -DDEBUG=$(DEBUG)

CFLAGS += -mmcu=$(MCU_TYPE) -DF_CPU=$(F_CPU)
CFLAGS += -O2 -ffunction-sections -fdata-sections  
CFLAGS += -mrelax
CFLAGS += -std=c99 -ggdb

CFLAGS += -W -Wall -Wstrict-prototypes -Wundef -Warray-bounds -Wformat -Wmissing-braces -Wreturn-type -Wextra -Wno-enum-compare
CFLAGS += $(INCLUDES)

BINARY_NAME:=firmware

all: init $(OBJS) $(BINARY_NAME).elf $(BINARY_NAME).lss $(BINARY_NAME).hex $(BINARY_NAME).eep stat 

init:
	mkdir -p $(DIR_OUT)
	
clean:
	rm -rf $(DIR_OUT)

stat: $(BINARY_NAME).elf
	$(SIZE) --format=avr $(DIR_OUT)/$(BINARY_NAME).elf

burn: $(BINARY_NAME).hex
	$(AVRDUDE) -C /etc/avrdude.conf -p t13 -P usb -c usbasp -u -U hfuse:w:0xF9:m -U lfuse:w:0x7A:m 
	$(AVRDUDE) -C /etc/avrdude.conf -p t13 -P usb -c usbasp -U flash:w:$(DIR_OUT)/$(BINARY_NAME).hex:i 

$(BINARY_NAME).elf: $(OBJS)
	@echo "Building binary... $(BINARY_NAME).elf"
	$(CC) -Wl,--gc-sections -Wl,-Map,$(DIR_OUT)/$(BINARY_NAME).map $(CFLAGS) -o $(DIR_OUT)/$(BINARY_NAME).elf $(OBJS) $(LDFLAGS)

$(BINARY_NAME).lss: $(BINARY_NAME).elf
	@echo "Creating Extended Listing... $(BINARY_NAME).lss"
	$(OBJDUMP) -h -S $(DIR_OUT)/$(BINARY_NAME).elf  > $(DIR_OUT)/$(BINARY_NAME).lss
	
$(BINARY_NAME).hex: $(BINARY_NAME).elf
	@echo "Create Flash image (ihex format)... $(BINARY_NAME).hex"
	$(OBJCOPY) -R .eeprom -O ihex $(DIR_OUT)/$(BINARY_NAME).elf $(DIR_OUT)/$(BINARY_NAME).hex
	$(OBJCOPY) -R .eeprom -O binary $(DIR_OUT)/$(BINARY_NAME).elf $(DIR_OUT)/$(BINARY_NAME).bin
	
$(BINARY_NAME).eep: $(BINARY_NAME).elf
	@echo "Create eeprom image (ihex format)... $(BINARY_NAME).eep"
	$(OBJCOPY) -j .eeprom -O ihex $(DIR_OUT)/$(BINARY_NAME).elf  $(DIR_OUT)/$(BINARY_NAME).eep
	
$(DIR_OUT)/%.c.o: $(DIR_SRC)/%.c
	@echo "Building $@"
	mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) -o $@ $<
	
$(DIR_OUT)/%.S.o: $(DIR_SRC)/%.S
	@echo "Building $@"
	mkdir -p `dirname $@`
	$(CC) -c -x assembler-with-cpp $(CFLAGS) -o $@  $< 
