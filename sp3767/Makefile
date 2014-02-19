CURRENT_DIR=$(shell pwd)

ifeq ($(CROSS_COMPILE),)
 CROSS_COMPILE=avr-
endif

ifneq ($(V),1)
 MAKEFLAGS += --silent
endif

ifeq ($(MCU_TYPE),)
 MCU_TYPE := atmega328p
endif

F_CPU := 16000000UL

CC       = $(CROSS_COMPILE)gcc
AS       = $(CROSS_COMPILE)as
OBJDUMP  = $(CROSS_COMPILE)objdump
OBJCOPY  = $(CROSS_COMPILE)objcopy
SIZE     = $(CROSS_COMPILE)size

BINARY_NAME := lmeter

DIR_INC := $(CURRENT_DIR)/inc
DIR_SRC := $(CURRENT_DIR)/src
DIR_OUT := $(CURRENT_DIR)/out

OBJS := \
  $(DIR_OUT)/main.o \
  $(DIR_OUT)/debug.o \
  $(DIR_OUT)/drv/i2c.o \
  $(DIR_OUT)/drv/tea5767.o
	
INCLUDES = -Iinclude

ifeq ($(DEBUG), 1)
   CFLAGS += -DENABLE_DEBUG
endif

# -mcall-prologues
CFLAGS += -mmcu=$(MCU_TYPE) -DF_CPU=$(F_CPU)
CFLAGS += -Os -fno-split-wide-types -ffunction-sections -fdata-sections  
CFLAGS += -fshort-enums -fpack-struct -mrelax    
CFLAGS += -std=c99 -ggdb

CFLAGS += -W -Wall -Wstrict-prototypes -Wundef -Warray-bounds -Wformat -Wmissing-braces -Wreturn-type -Wextra -Wno-enum-compare
CFLAGS += $(INCLUDES)


all: init $(OBJS) $(BINARY_NAME).elf $(BINARY_NAME).lss $(BINARY_NAME).hex $(BINARY_NAME).eep stat

init:
	mkdir -p $(DIR_OUT)
	
burn: $(BINARY_NAME).hex
	tools/burner.elf -w -i $(DIR_OUT)/$(BINARY_NAME).bin -c -r
	
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
	
stat: $(BINARY_NAME).elf
	$(SIZE) --format=avr $(DIR_OUT)/$(BINARY_NAME).elf
	
$(DIR_OUT)/%.o: $(DIR_SRC)/%.c
	@echo "Building $@"
	mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) -o $@ $<
	
$(DIR_OUT)/%.S.o: $(DIR_SRC)/%.S
	@echo "Building $@"
	mkdir -p `dirname $@`
	$(CC) -c -x assembler-with-cpp $(CFLAGS) -o $@  $< 
	
clean:
	rm -rf $(DIR_OUT)
