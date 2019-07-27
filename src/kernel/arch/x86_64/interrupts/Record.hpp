#ifndef RECORD_HPP
#define RECORD_HPP

#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>

template<typename T> class __attribute__((__packed__)) Record
{
private:
    uint16_t size;
    T *address;

public:
    Record(T *table);
    void load();
};

template class Record<InterruptDescriptorTable>;
template class Record<GlobalDescriptorTable>;

#endif // RECORD_HPP
