.PHONY: clean install

CXX_SOURCES = $(shell find . -type f -name '*.cpp' -not -path './arch/*')
CXX_HEADERS = $(shell find . -type f -name '*.hpp' -not -path './arch/*')
CXX_SOURCES += $(shell find arch/$(ARCH) -type f -name '*.cpp')
CXX_HEADERS += $(shell find arch/$(ARCH) -type f -name '*.hpp')
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = memory/dlmalloc.c
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))

ASM_SOURCES = $(shell find arch/$(ARCH) -type f -name '*.asm')
ASM_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))
CRTI_OBJ = $(SHBUILD_BUILD_DIR)/arch/$(ARCH)/lib/crt/crti.o
CRTN_OBJ = $(SHBUILD_BUILD_DIR)/arch/$(ARCH)/lib/crt/crtn.o
ASM_REGULER_OBJS = $(filter-out $(CRTI_OBJ), $(filter-out $(CRTN_OBJ), $(ASM_OBJS)))

TARGET = $(SHBUILD_BUILD_DIR)/SHKERNEL.BIN


COMMON_FLAGS = -O0 -g -fsanitize=undefined -ffreestanding -fno-stack-protector -Wall -Wextra -Wpedantic -Werror -I$(PWD)/arch/$(ARCH) -I$(PWD)
COMMON_FLAGS += $(KERNEL_CFLAGS_$(ARCH))

CFLAGS = $(COMMON_FLAGS) -std=c99 -Wno-expansion-to-defined -include memory/dlmalloc-glue.hpp
CXXFLAGS = $(COMMON_FLAGS) -std=c++17 -fno-rtti -fno-exceptions

LDFLAGS = -O2 -ffreestanding -nostdlib -static -L$(PWD)/arch/$(ARCH)/lib/crt -z max-page-size=0x1000 -Wl,--build-id=none -T arch/$(ARCH)/linker.ld -nostdlib -lgcc


CXX = $(ARCH)-elf-g++
CC = $(ARCH)-elf-gcc


CRTBEGIN_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)


$(TARGET): $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(CRTBEGIN_OBJ) $(CRTI_OBJ) $(C_OBJS) $(CXX_OBJS) $(ASM_REGULER_OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

$(SHBUILD_BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.c $(C_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	nasm -f elf64 -o $@ $<

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(TARGET)

install:
	# do nothing
