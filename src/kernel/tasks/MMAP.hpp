#ifndef MMAP_HPP
#define MMAP_HPP

#include <common/common.hpp>
#include <tasks/Object.hpp>

class Task;

class MMAP {
private:
    UUID target;
    usize start_address;
    usize offset;
    usize length;
    usize result;
    u32 protection;
    u32 flags;
    u32 error;

    bool addressLengthValid();
    bool addressLengthAvailable();

public:
    void perform(Task &task);
};

#endif // MMAP_HPP
