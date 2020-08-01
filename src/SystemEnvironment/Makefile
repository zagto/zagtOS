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

EMBEDDED_BINARIES = $(SHBUILD_ROOT)/build/ACPIHAL/ACPIHAL $(SHBUILD_ROOT)/build/PCIController/PCIController $(SHBUILD_ROOT)/build/AHCIDriver/AHCIDriver
BINARY_OBJS = $(SHBUILD_BUILD_DIR)/binaries.o

TARGET = $(SHBUILD_BUILD_DIR)/SystemEnvironment


COMMON_FLAGS = -O2 -Wall -Wextra -Wpedantic -Werror -I$(PWD)/arch/$(ARCH) -I$(PWD)

CFLAGS = $(COMMON_FLAGS) -std=c18
CXXFLAGS = $(COMMON_FLAGS) -std=c++17

LDFLAGS = -O2 -s


CXX = $(ARCH)-zagto-zagtos-g++
CC = $(ARCH)-zagto-zagtos-gcc
LD = $(ARCH)-zagto-zagtos-ld


$(TARGET): $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(BINARY_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^ -lzagtos++

$(SHBUILD_BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.c $(C_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/binaries.o: $(EMBEDDED_BINARIES)
	mkdir -p $(dir $@)
	# copy all binaries to the same directory and link there to genearate
	# symbol names without the whole path
	cp $(EMBEDDED_BINARIES) $(SHBUILD_BUILD_DIR)
	cd $(SHBUILD_BUILD_DIR) && $(LD) -b binary -o $@ $(notdir $(EMBEDDED_BINARIES))

$(SHBUILD_BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	nasm -f elf64 -o $@ $<

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(BINARY_OBJS) $(TARGET)

install:
	# do nothing
