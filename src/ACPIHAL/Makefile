.PHONY: clean install

ACPICA_ENABLED_PARTS=acpica/source/components/dispatcher acpica/source/components/events acpica/source/components/executer acpica/source/components/hardware acpica/source/include acpica/source/components/namespace acpica/source/components/parser acpica/source/components/resources acpica/source/components/tables acpica/source/components/utilities

CXX_SOURCES = $(shell find -L . -type f -name '*.cpp' -not -path './acpica/*')
CXX_HEADERS = $(shell find -L . -type f -name '*.hpp' -not -path './acpica/*')
CXX_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

C_SOURCES = $(shell find -L . -type f -name '*.c' -not -path './acpica/*')
C_HEADERS = $(shell find -L . -type f -name '*.h' -not -path './acpica/*')
# ACPICA C Code
C_SOURCES += $(shell find -L $(ACPICA_ENABLED_PARTS) -type f -name '*.c')
C_HEADERS += $(shell find -L $(ACPICA_ENABLED_PARTS) -type f -name '*.h')
C_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/, $(C_SOURCES:.c=.o))


TARGET = $(SHBUILD_BUILD_DIR)/ACPIHAL


COMMON_FLAGS = -Os -g -Wall -Wextra -Wno-pedantic -Werror -I$(PWD) -I$(PWD)/acpica/source/include -ffunction-sections -fdata-sections

# ACPICA C code will trigger warnings, since our code is mostly C++, just hide them
CFLAGS = $(COMMON_FLAGS) -std=c18 -Wno-unused-parameter -Wno-format-truncation -Wno-stringop-truncation -Wno-format -Wno-pedantic -Wno-error=dangling-pointer
CXXFLAGS = $(COMMON_FLAGS) -std=c++2a

LDFLAGS = -Os


CXX = $(ARCH)-zagto-zagtos-g++
CC = $(ARCH)-zagto-zagtos-gcc


$(TARGET): $(CXX_OBJS) $(C_OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^ -lzagtos++

$(SHBUILD_BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS) $(C_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SHBUILD_BUILD_DIR)/%.o: %.c $(C_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(TARGET)

install:
	# do nothing
