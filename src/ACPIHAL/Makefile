.PHONY: clean install

CXX_SOURCES = $(shell find -L . -type f -name '*.cpp' -not -path './acpica/*')
CXX_HEADERS = $(shell find -L . -type f -name '*.hpp' -not -path './acpica/*')
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = $(shell find -L . -type f -name '*.c' -not -path './acpica/*')
C_HEADERS = $(shell find -L . -type f -name '*.h' -not -path './acpica/*')
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))


TARGET = $(SHBUILD_BUILD_DIR)/ACPIHAL


COMMON_FLAGS = -O2 -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-error=format-truncation -Wno-error=format -I$(PWD) -I$(PWD)/selection/include

CFLAGS = $(COMMON_FLAGS) -std=c18
CXXFLAGS = $(COMMON_FLAGS) -std=c++17

LDFLAGS = -O2


CXX = $(ARCH)-zagto-zagtos-g++
CC = $(ARCH)-zagto-zagtos-gcc


$(TARGET): $(CXX_OBJS) $(C_OBJS)
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
	rm -f $(CXX_OBJS) $(C_OBJS) $(TARGET)

install:
	# do nothing
