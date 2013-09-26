include $(PROJECT_ROOT)/Makefile.conf
include Makefile.conf


SRC_C  := $(shell find $(DIR_SRC) -name '*.c' | sort)
SRC_AS := $(shell find $(DIR_SRC) -name '*.S' | sort)

TMP_C  := $(foreach file, $(SRC_C), $(shell echo $(file) | sed -e 's|$(DIR_SRC)\/|$(DIR_OUT)\/|'))
TMP_AS := $(foreach file, $(SRC_AS), $(shell echo $(file) | sed -e 's|$(DIR_SRC)\/|$(DIR_OUT)\/|')) 

OBJ_SUFFIX := .c.o
OBJ_C := $(foreach file, $(TMP_C), $(shell echo $(file) | sed -e 's|\.c$$|$(OBJ_SUFFIX)|')) 
DEP_C := $(foreach file, $(TMP_C), $(shell echo $(file) | sed -e 's|\.c$$|.c.d|')) 

OBJ_SUFFIX := .S.o
OBJ_AS := $(foreach file, $(TMP_AS), $(shell echo $(file) | sed -e 's|\.S$$|$(OBJ_SUFFIX)|')) 
DEP_AS := 

SRC := $(SRC_AS) $(SRC_C)
OBJ := $(OBJ_AS) $(OBJ_C)
DEP := $(DEP_AS) $(DEP_C)

LDFLAGS += $(LIB_MCU)

APPLICATION_NAME := usb-jboot

CFLAGS += -I$(DIR_INC) -I$(DIR_INC)/bootloader/vusb

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

init:
	$(MKDIR) $(DIR_OUT)

all: init $(OBJ) $(DIR_OUT)/$(APPLICATION_NAME).elf $(DIR_OUT)/$(APPLICATION_NAME).lss $(DIR_OUT)/$(APPLICATION_NAME).hex $(DIR_OUT)/$(APPLICATION_NAME).eep $(DIR_OUT)/$(APPLICATION_NAME).bin

clean:
	$(RM) -rf $(DIR_OUT)

burn: all
ifeq ($(MCU),atmega328p)
	avrdude -p m328p -P usb -c usbasp -u -U efuse:w:0x07:m -U hfuse:w:0xD8:m -U lfuse:w:0xF7:m
	avrdude -p m328p -P usb -c usbasp -U flash:w:$(DIR_OUT)/$(APPLICATION_NAME).hex:i 
else
	$(ECHO) "Cannot burn MCU '$(MCU)'"
endif

#/usr/bin/avrdude -C /home/jarko/Pulpit/avr8-burn-o-mat/avrdude.conf.txt -p m328p -P usb -c usbasp  -u -U efuse:w:0x07:m -U hfuse:w:0xD8:m -U lfuse:w:0xF7:m

-include $(PROJECT_ROOT)/Makefile.rules
-include Makefile.rules
