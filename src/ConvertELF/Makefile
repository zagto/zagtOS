.PHONY: clean install

CXX_SOURCES = $(shell find . -name '*.cpp')
CXX_HEADERS = $(shell find . -name '*.hpp')
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = $(shell find . -name '*.c')
C_HEADERS = $(shell find . -name '*.h')
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))

ASM_SOURCES = $(shell find arch/$(ARCH) -name '*.asm')
ASM_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))


TARGET = $(SHBUILD_BUILD_DIR)/ConvertELF


COMMON_FLAGS = -O2 -Wall -Wextra -Wpedantic -Werror -I$(PWD)/arch/$(ARCH) -I$(PWD) -DZAGTOS_ARCH_$(ARCH)=1

CFLAGS = $(COMMON_FLAGS) -std=c18
CXXFLAGS = $(COMMON_FLAGS) -std=c++2a

LDFLAGS = -O2 -s


CXX = g++
CC = gcc
LD = ld


$(TARGET): $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^

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
