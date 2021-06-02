#pragma once

#include <interrupts/InterruptDescriptorTableEntry.hpp>
#include <interrupts/ContextSwitch.hpp>

class InterruptDescriptorTable {
private:
    struct __attribute__((__packed__)) IDTRStruct {
        uint16_t size;
        InterruptDescriptorTableEntry *address;
    };

    InterruptDescriptorTableEntry idt[NUM_INTERRUPT_SERIVICE_ROUTINES];
    IDTRStruct idtr;

public:
    InterruptDescriptorTable();

    void load();
};
