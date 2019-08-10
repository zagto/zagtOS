#ifndef MAPPINGOPERATION_HPP
#define MAPPINGOPERATION_HPP

#include <common/common.hpp>
#include <tasks/Object.hpp>

class Task;

class MappingOperation {
protected:
    size_t start_address;
    size_t length;

    bool addressLengthValid();
    MappingOperation(size_t start_address, size_t length) :
        start_address{start_address},
        length{length} {}
    MappingOperation() {}

public:
    uint32_t error;
};

class MUnmap : public MappingOperation {
public:
    void perform(Task &task);
    MUnmap(size_t start_address, size_t length) :
        MappingOperation(start_address, length) {}
};

class MMap : public MappingOperation {
    uint32_t flags;
    size_t offset;
    UUID target;
    size_t result;
    uint32_t protection;

public:
    void perform(Task &task);
    /* only called before USO is loaded, does nothing */
    MMap() {}
};

#endif // MAPPINGOPERATION_HPP