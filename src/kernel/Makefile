.PHONY: clean install libsupcxx_setup

CXX_SOURCES = $(shell find -L . -type f -name '*.cpp' -follow -not -path './arch/*')
CXX_HEADERS = $(shell find -L . -type f -name '*.hpp' -follow -not -path './arch/*')
CXX_SOURCES += $(shell find -L arch/$(ARCH) -type f -name '*.cpp' -follow)
CXX_HEADERS += $(shell find -L arch/$(ARCH) -type f -name '*.hpp' -follow)
CXX_HEADERS += $(shell find stdlib -type f -follow)
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = memory/dlmalloc.c
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))

LIBSUPCXX_SOURCES = $(shell ls ../gcc/libstdc++-v3/libsupc++/*.cc | grep -v atexit_thread.cc)
LIBSUPCXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/libsupc++/, $(notdir $(LIBSUPCXX_SOURCES:.cc=.o)))

ASM_SOURCES = $(shell find arch/$(ARCH) -type f -name '*.asm')
ASM_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))
CRTI_OBJ = $(SHBUILD_BUILD_DIR)/arch/$(ARCH)/lib/crt/crti.o
CRTN_OBJ = $(SHBUILD_BUILD_DIR)/arch/$(ARCH)/lib/crt/crtn.o
ASM_REGULER_OBJS = $(filter-out $(CRTI_OBJ), $(filter-out $(CRTN_OBJ), $(ASM_OBJS)))

TARGET = $(SHBUILD_BUILD_DIR)/ZAGTKERN.ZBN

COMMON_FLAGS = -Os -g  -ffreestanding -fno-stack-protector -Wall -Wextra -Wpedantic -Werror -fsanitize=undefined -I$(PWD)/arch/$(ARCH) -I$(PWD) -I$(PWD)/stdlib
COMMON_FLAGS += $(KERNEL_CFLAGS_$(ARCH))

CFLAGS = $(COMMON_FLAGS) -std=c99 -Wno-expansion-to-defined
CXXFLAGS = $(COMMON_FLAGS) -std=c++2a

LDFLAGS = -O2 -ffreestanding -nostdlib -static -L$(PWD)/arch/$(ARCH)/lib/crt -z max-page-size=0x1000 -Wl,--build-id=none -T arch/$(ARCH)/linker.ld -nostdlib


CXX = $(ARCH)-elf-g++
CC = $(ARCH)-elf-gcc
export STRIP = $(ARCH)-elf-strip
CONVERTELF = $(SHBUILD_ROOT)/build/ConvertELF/ConvertELF


CRTBEGIN_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

$(TARGET): $(SHBUILD_BUILD_DIR)/kernel.elf
	$(CONVERTELF) $< $@ --no-add-stack

$(SHBUILD_BUILD_DIR)/kernel.elf: $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(LIBSUPCXX_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(C_OBJS) $(LIBSUPCXX_OBJS) $(CXX_OBJS) $(ASM_REGULER_OBJS) $(SHBUILD_BUILD_DIR)/../gcc/kernel-x86_64/gcc/libgcc.a $(CRTEND_OBJ) $(CRTN_OBJ)

$(SHBUILD_BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.c $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	nasm -f elf64 -o $@ $<


# detect libsupc++ by .cc extension

$(SHBUILD_BUILD_DIR)/libsupc++/%.o: ../gcc/libstdc++-v3/libsupc++/%.cc $(CXX_HEADERS) libsupcxx_setup
	$(CXX) $(CXXFLAGS) -I $(SHBUILD_BUILD_DIR)/libsupc++/include/ -I ../gcc/libstdc++-v3/include/c_std/ -I $(SHBUILD_BUILD_DIR)/libsupc++ -I ../gcc/libstdc++-v3/include -I ../gcc/libstdc++-v3/include/std -c -o $@ $<

libsupcxx_setup:
	mkdir -p $(SHBUILD_BUILD_DIR)/libsupc++/include/bits
	cp -r ../gcc/libstdc++-v3/libsupc++/* $(SHBUILD_BUILD_DIR)/libsupc++

	bash -c "(cat ../gcc/libstdc++-v3/include/bits/c++config | sed 's/#define _GLIBCXX_HAVE_ATTRIBUTE_VISIBILITY/& 1/' | sed 's/#define _GLIBCXX_USE_DUAL_ABI/& 1/' | sed 's/#define _GLIBCXX_USE_CXX11_ABI/& 1/' | sed 's/#define _GLIBCXX_INLINE_VERSION/& 0/' | sed 's/#define _GLIBCXX_USE_ALLOCATOR_NEW/& 1/' && echo '#endif') > $(SHBUILD_BUILD_DIR)/libsupc++/include/bits/c++config.h"
	cp ../gcc/libgcc/unwind-pe.h $(SHBUILD_BUILD_DIR)/libsupc++/include/
	cp ../gcc/libstdc++-v3/config/os/generic/os_defines.h $(SHBUILD_BUILD_DIR)/libsupc++/include/bits
	cp ../gcc/libstdc++-v3/config/cpu/generic/*.h $(SHBUILD_BUILD_DIR)/libsupc++/include/bits/
	cp $(SHBUILD_BUILD_DIR)/libsupc++/*.h $(SHBUILD_BUILD_DIR)/libsupc++/include/bits

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(LIBSUPCXX_OBJS) $(TARGET)

install:
	# do nothing
