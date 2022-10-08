.PHONY: all all-x86_64 all-aarch64 clean-x86_64 clean-aarch64 clean install

all: all-$(ARCH)
clean: clean-$(ARCH)

all-x86_64:
	make -f Makefile.efi all-efi
	make -f Makefile.multiboot all-multiboot

clean-x86_64:
	make -f Makefile.efi clean-efi
	make -f Makefile.multiboot clean-multiboot

all-aarch64:
	# should be possible but GNU EFI won't build for ARM for some reason
	#make -f Makefile.efi all-efi
	make -f Makefile.android all-android

clean-x86_64:
	#make -f Makefile.efi clean-efi
	make -f Makefile.android clean-android

install: # do nothing

