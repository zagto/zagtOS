.PHONY: clean install

CXX_SOURCES = $(shell find -L . -type f -name '*.cpp' -follow -not -path './arch/*')
CXX_HEADERS = $(shell find -L . -type f -name '*.hpp' -follow -not -path './arch/*')
CXX_SOURCES += $(shell find -L arch/$(ARCH) -type f -name '*.cpp' -follow)
CXX_HEADERS += $(shell find -L arch/$(ARCH) -type f -name '*.hpp' -follow)
CXX_HEADERS += $(shell find stdlib -type f -follow)
CXX_OBJS = $(addprefix $(BUILD_DIR)/, $(CXX_SOURCES:.cpp=.o))

ASM_SOURCES = $(shell find arch/$(ARCH) -type f -name '*.asm')
ASM_OBJS = $(addprefix $(BUILD_DIR)/, $(ASM_SOURCES:.asm=.o))
CRTI_OBJ = $(BUILD_DIR)/arch/$(ARCH)/crt/crti.o
CRTN_OBJ = $(BUILD_DIR)/arch/$(ARCH)/crt/crtn.o
ASM_REGULER_OBJS = $(filter-out $(CRTI_OBJ), $(filter-out $(CRTN_OBJ), $(ASM_OBJS)))

TARGET = $(BUILD_DIR)/kernel-loader-common.a

PWD = $(shell pwd)

# CXXFLAGS should be exported when calling this Makefile

CXX = $(ARCH)-elf-g++
AR = $(ARCH)-elf-gcc-ar

all: $(TARGET) $(CRTI_OBJ) $(CRTN_OBJ)

$(TARGET): $(CXX_OBJS) $(C_OBJS) $(ASM_REGULER_OBJS)
	$(AR) -rcs $(TARGET) $(CXX_OBJS) $(C_OBJS) $(ASM_REGULER_OBJS)

$(BUILD_DIR)/%.o: %.cpp $(CXX_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	$(AS_$(ARCH)) -o $@ $<

clean:
	rm -f $(CXX_OBJS) $(C_OBJS) $(ASM_OBJS) $(TARGET)

install:
	# do nothing

