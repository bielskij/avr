F_CPU := 16000000ULL

# BOOTSZ1 | BOOTSZ0 | Boot Size | Pages | Application Flash Section | Boot Loader Flash Section |  End Application Section | Boot Reset Address (Start Boot Loader Section)
# 1 1  256 words  4 0x0000 - 0x3EFF 0x3F00 - 0x3FFF 0x3EFF 0x3F00
# 1 0  512 words  8 0x0000 - 0x3DFF 0x3E00 - 0x3FFF 0x3DFF 0x3E00
# 0 1 1024 words 16 0x0000 - 0x3BFF 0x3C00 - 0x3FFF 0x3BFF 0x3C00
# 0 0 2048 words 32 0x0000 - 0x37FF 0x3800 - 0x3FFF 0x37FF 0x3800

# 7800 - 2kB 
# 7000 - 4kB 
BOOTLOADER_SECTION_START_ADDRESS=0x7000

CFLAGS += -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DBOOTLOADER_SECTION_START_ADDRESS=$(BOOTLOADER_SECTION_START_ADDRESS)

# -fomit-frame-pointer        When possible do not generate stack frames
# -fpack-struct               Pack structure members together without holes
# -fjump-tables               Use jump tables for sufficiently large switch
#                             statements

CFLAGS += -Os -mtiny-stack -nostartfiles -fno-split-wide-types
CFLAGS += -fshort-enums -fpack-struct -mrelax
CFLAGS += -g -W -Wall -Wstrict-prototypes -Wundef -Warray-bounds -Wformat -Wmissing-braces -Wreturn-type -Wextra

CFLAGS += -I$(DIR_CONFIG)/$(MCU)/

ifeq ($(MCU_DISABLE_SECTIONS_OPTIMALIZATION),)
   CFLAGS  += -ffunction-sections -fdata-sections
   LDFLAGS += -Wl,--gc-sections
endif

LDFLAGS += -Wl,--section-start=.text=$(BOOTLOADER_SECTION_START_ADDRESS) -Wl,-Map,$(DIR_OUT)/$(APPLICATION_NAME).map,--cref
