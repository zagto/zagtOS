.PHONY: clean install

ARCH = x86_64

C_SOURCES = $(shell find . -type f -name '*.c')
C_HEADERS = $(shell find . -type f -name '*.h')
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))

ASM_SOURCES = $(shell find . -type f -name '*.asm')
ASM_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))

SOFILE = $(SHBUILD_BUILD_DIR)/loader.so
EFIFILE = $(SHBUILD_BUILD_DIR)/BOOTX64.EFI

EFI_INCLUDE_BASE = /usr/include/efi
EFI_INCLUDES = -I$(EFI_INCLUDE_BASE) -I$(EFI_INCLUDE_BASE)/$(ARCH) -I$(EFI_INCLUDE_BASE)/protocol
EFI_LIB_BASE = /usr/lib64
EFI_CRT_OBJS = $(EFI_LIB_BASE)/crt0-efi-$(ARCH).o
EFI_LDS = $(EFI_LIB_BASE)/elf_$(ARCH)_efi.lds

CFLAGS = $(EFI_INCLUDES) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -Werror -I$(PWD)
ifeq ($(ARCH),x86_64)
    CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB_BASE) $(EFI_CRT_OBJS)

OBJCOPY_SECTIONS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .reloc


$(EFIFILE): $(SOFILE)
	objcopy $(OBJCOPY_SECTIONS) --target=efi-app-$(ARCH) $^ $@

$(SOFILE): $(C_OBJS) $(ASM_OBJS)
	ld $(LDFLAGS) $^ -o $@ -lefi -lgnuefi

$(SHBUILD_BUILD_DIR)/%.o: %.asm
	nasm -f elf64 -o $@ $^

$(SHBUILD_BUILD_DIR)/%.o: %.c $(C_HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(C_OBJS) $(ASM_OBJS) $(EFIFILE) $(SOFILE)

install:
	# do nothing
