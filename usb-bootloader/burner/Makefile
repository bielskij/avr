include $(PROJECT_ROOT)/Makefile.conf


SRC := $(shell find $(DIR_SRC) -name '*.c' | sort)
TMP := $(foreach file, $(SRC), $(shell echo $(file) | sed -e 's|$(DIR_SRC)\/|$(DIR_OUT)\/|')) 

OBJ_SUFFIX := .c.o
OBJ := $(foreach file, $(TMP), $(shell echo $(file) | sed -e 's|\.c$$|$(OBJ_SUFFIX)|')) 
DEP := $(foreach file, $(TMP), $(shell echo $(file) | sed -e 's|\.c$$|.c.d|')) 

ifeq ($(DEBUG), 1)
  CFLAGS += -DENABLE_DEBUG -g
endif

CFLAGS += -I$(DIR_INC) -I../bootloader/inc
LDFLAGS += -lusb 


all: $(DIR_OUT)/burner.elf

clean:
	rm -rf $(DIR_OUT)
	
$(DIR_OUT)/%.elf: $(OBJ)
	@echo "Building binary... $(APPLICATION_NAME).elf"
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
	
include $(PROJECT_ROOT)/Makefile.rules
