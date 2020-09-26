.PHONY: clean install

CXX_SOURCES = $(shell find . -type f -name '*.cpp' -not -path './arch/*')
CXX_HEADERS = $(shell find . -type f -name '*.hpp' -not -path './arch/*')
CXX_SOURCES += $(shell find arch/$(ARCH) -type f -name '*.cpp')
CXX_HEADERS += $(shell find arch/$(ARCH) -type f -name '*.hpp')
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = $(shell find . -type f -name '*.c' -not -path './arch/*')
C_HEADERS = $(shell find . -type f -name '*.h' -not -path './arch/*')
C_SOURCES += $(shell find arch/$(ARCH) -type f -name '*.c')
C_HEADERS += $(shell find arch/$(ARCH) -type f -name '*.h')
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))

ASM_SOURCES = $(shell find arch/$(ARCH) -type f -name '*.asm')
ASM_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))


TARGET = $(SHBUILD_BUILD_DIR)/libzagtos++.a


COMMON_FLAGS = -O2 -Wall -Wextra -Wpedantic -Werror -I$(PWD)/arch/$(ARCH) -I$(PWD)

CFLAGS = $(COMMON_FLAGS) -std=c18
CXXFLAGS = $(COMMON_FLAGS) -std=c++2a

LDFLAGS = -O2


CXX = $(ARCH)-zagto-zagtos-g++
CC = $(ARCH)-zagto-zagtos-gcc
LD = $(ARCH)-zagto-zagtos-ld
AR = $(ARCH)-zagto-zagtos-gcc-ar


$(TARGET): $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS)
	mkdir -p $(dir $@)
	$(AR) rcs $@ $^

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
	cp $(CXX_HEADERS) "$(SYSROOT)/include/zagtos/"
	cp $(TARGET) "$(SYSROOT)/lib/"
