F_CPU := 16000000ULL

BOOTLOADER_SECTION_START_ADDRESS=0x7000

CFLAGS += -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DBOOTLOADER_SECTION_START_ADDRESS=$(BOOTLOADER_SECTION_START_ADDRESS)

# -fomit-frame-pointer        When possible do not generate stack frames
# -fpack-struct               Pack structure members together without holes
# -fjump-tables               Use jump tables for sufficiently large switch
#                             statements

CFLAGS += -Os -mtiny-stack -nostartfiles -fno-split-wide-types
CFLAGS += -fshort-enums -fpack-struct -mrelax 
CFLAGS += -g -Wall -Wundef -Warray-bounds -Wformat -Wmissing-braces -Wreturn-type

CFLAGS += -I$(DIR_CONFIG)/$(MCU)/

ifeq ($(MCU_DISABLE_SECTIONS_OPTIMALIZATION),)
   CFLAGS  += -ffunction-sections -fdata-sections
   LDFLAGS += -Wl,--gc-sections
endif

LDFLAGS += -Wl,--section-start=.text=$(BOOTLOADER_SECTION_START_ADDRESS) -Wl,-Map,$(DIR_OUT)/$(APPLICATION_NAME).map,--cref
