#ifndef INTERRUPTDESCRIPTORTABLE_HPP
#define INTERRUPTDESCRIPTORTABLE_HPP

#include <interrupts/InterruptDescriptorTableEntry.hpp>
#include <interrupts/ContextSwitch.hpp>

class __attribute__((__packed__)) InterruptDescriptorTable {
private:
    InterruptDescriptorTableEntry entries[NUM_INTERRUPT_SERIVICE_ROUTINES];

public:
    InterruptDescriptorTable();
};

#endif // INTERRUPTDESCRIPTORTABLE_HPP
