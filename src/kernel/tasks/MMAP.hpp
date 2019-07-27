#ifndef MMAP_HPP
#define MMAP_HPP

#include <common/common.hpp>
#include <tasks/Object.hpp>

class Task;

class MMAP {
private:
    UUID target;
    size_t start_address;
    size_t offset;
    size_t length;
    size_t result;
    uint32_t protection;
    uint32_t flags;
    uint32_t error;

    bool addressLengthValid();
    bool addressLengthAvailable();

public:
    void perform(Task &task);
};

#endif // MMAP_HPP
