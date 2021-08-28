.PHONY: all clean install

all:
	make -f Makefile.efi all-efi
	make -f Makefile.multiboot all-multiboot

clean: clean-efi clean-multiboot
	make -f Makefile.efi clean-efi
	make -f Makefile.multiboot clean-multiboot

install: # do nothing

