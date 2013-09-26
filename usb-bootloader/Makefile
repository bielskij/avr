include Makefile.conf

PROJECT_ROOT = $(shell pwd)

# Export all previously defined variables to the submakefiles.	
export

all:
	$(MAKE) -C $(APPLICATION) all
 
clean:
	$(MAKE) -C $(APPLICATION) clean

burn:
	$(MAKE) -C bootloader burn