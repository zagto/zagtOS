.PHONY: clean install

CXX_SOURCES = $(shell find -L . -type f -name '*.cpp' -follow -not -path './arch/*')
CXX_HEADERS = $(shell find -L . -type f -name '*.hpp' -follow -not -path './arch/*')
CXX_SOURCES += $(shell find -L arch/$(ARCH) -type f -name '*.cpp' -follow)
CXX_HEADERS += $(shell find -L arch/$(ARCH) -type f -name '*.hpp' -follow)
CXX_HEADERS += $(shell find stdlib -type f -follow)
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = memory/dlmalloc.c
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))

ASM_SOURCES = $(shell find arch/$(ARCH) -type f -name '*.asm')
ASM_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))
CRTI_OBJ = $(SHBUILD_BUILD_DIR)/arch/$(ARCH)/lib/crt/crti.o
CRTN_OBJ = $(SHBUILD_BUILD_DIR)/arch/$(ARCH)/lib/crt/crtn.o
ASM_REGULER_OBJS = $(filter-out $(CRTI_OBJ), $(filter-out $(CRTN_OBJ), $(ASM_OBJS)))

TARGET = $(SHBUILD_BUILD_DIR)/ZAGTKERN.ZBN


COMMON_FLAGS = -Os -g -fsanitize=undefined -ffreestanding -fno-stack-protector -Wall -Wextra -Wpedantic -Werror -I$(PWD)/arch/$(ARCH) -I$(PWD) -I$(PWD)/stdlib
COMMON_FLAGS += $(KERNEL_CFLAGS_$(ARCH))

CFLAGS = $(COMMON_FLAGS) -std=c99 -Wno-expansion-to-defined -include memory/dlmalloc-glue.hpp
CXXFLAGS = $(COMMON_FLAGS) -std=c++2a -fno-rtti -fno-exceptions

LDFLAGS = -O2 -ffreestanding -nostdlib -static -L$(PWD)/arch/$(ARCH)/lib/crt -z max-page-size=0x1000 -Wl,--build-id=none -T arch/$(ARCH)/linker.ld -nostdlib -lgcc


CXX = $(ARCH)-elf-g++
CC = $(ARCH)-elf-gcc
export STRIP = $(ARCH)-elf-strip
CONVERTELF = $(SHBUILD_ROOT)/build/ConvertELF/ConvertELF


CRTBEGIN_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

$(TARGET): $(SHBUILD_BUILD_DIR)/kernel.elf
	$(CONVERTELF) $< $@

$(SHBUILD_BUILD_DIR)/kernel.elf: $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(CRTBEGIN_OBJ) $(CRTI_OBJ) $(C_OBJS) $(CXX_OBJS) $(ASM_REGULER_OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

$(SHBUILD_BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.c $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	nasm -f elf64 -o $@ $<

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(TARGET)

install:
	# do nothing
