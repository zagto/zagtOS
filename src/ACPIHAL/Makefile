.PHONY: clean install

CXX_SOURCES = $(shell find -L . -type f -name '*.cpp' -not -path './acpica/*')
CXX_HEADERS = $(shell find -L . -type f -name '*.hpp' -not -path './acpica/*')
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = $(shell find -L . -type f -name '*.c' -not -path './acpica/*')
C_HEADERS = $(shell find -L . -type f -name '*.h' -not -path './acpica/*')
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))


TARGET = $(SHBUILD_BUILD_DIR)/ACPIHAL


COMMON_FLAGS = -Os -Wall -Wextra -Wpedantic -Werror -I$(PWD) -I$(PWD)/selection/include -fsanitize=undefined -ffunction-sections -fdata-sections

# ACPICA C code will trigger warnings, since our code is mostly C++, just hide them
CFLAGS = $(COMMON_FLAGS) -std=c18 -Wno-unused-parameter -Wno-format-truncation -Wno-stringop-truncation -Wno-format 
CXXFLAGS = $(COMMON_FLAGS) -std=c++2a

LDFLAGS = -Os -s -fsanitize=undefined


CXX = $(ARCH)-zagto-zagtos-g++
CC = $(ARCH)-zagto-zagtos-gcc


$(TARGET): $(CXX_OBJS) $(C_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^ -lzagtos++

$(SHBUILD_BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.c $(C_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(TARGET)

install:
	# do nothing
