#ifndef MAPPINGOPERATION_HPP
#define MAPPINGOPERATION_HPP

#include <common/common.hpp>
#include <processes/UUID.hpp>

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
public:
    void perform(Process &process);
    MUnmap(size_t address, size_t _length) :
        MappingOperation(address, _length) {}
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
    UUID target;
    size_t result;
    uint32_t protection;

public:
    void perform(Process &process);
    /* only called before USO is loaded, does nothing */
    MMap() {}
};

#endif // MAPPINGOPERATION_HPP
