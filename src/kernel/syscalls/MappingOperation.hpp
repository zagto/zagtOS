#pragma once
#include <common/common.hpp>
#include <processes/UUID.hpp>

static const int32_t PROTECTION_READ = 1,
                     PROTECTION_WRITE = 2,
                     PROTECTION_EXECUTE = 4;
static const uint32_t MAP_SHARED = 0x01,
                      MAP_PRIVATE = 0x02,
                      MAP_FIXED = 0x10,
                      MAP_ANONYMOUS = 0x20,
                      MAP_WHOLE = 0x20000000;

class Process;

class MappingOperation {
protected:
    size_t startAddress;
    size_t length;

    bool addressLengthValid();
    MappingOperation(size_t address, size_t length) :
        startAddress{address},
        length{length} {}
    MappingOperation() {}

public:
    uint32_t error;
};

class MUnmap : public MappingOperation {
protected:
    bool wholeArea;

public:
    void perform(Process &process);
    MUnmap(size_t address, size_t _length, bool wholeArea) :
        MappingOperation(address, _length),
        wholeArea{wholeArea} {}
};

class MProtect : public MappingOperation {
private:
    uint32_t protection;
public:
    void perform(Process &process);
    MProtect(size_t address, size_t _length, uint32_t protection) :
        MappingOperation(address, _length),
        protection{protection} {}
};

class MMap : public MappingOperation {
    uint32_t flags;
    size_t offset;
    size_t result;
    uint32_t handle;
    uint32_t protection;

public:
    void perform(Process &process);
    /* only called before USO is loaded, does nothing */
    MMap() {}
};
