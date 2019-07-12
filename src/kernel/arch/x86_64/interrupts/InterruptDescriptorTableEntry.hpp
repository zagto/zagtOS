#ifndef INTERRUPTDESCRIPTORTABLEENTRY_HPP
#define INTERRUPTDESCRIPTORTABLEENTRY_HPP

#include <common/common.hpp>
#include <interrupts/ContextSwitch.hpp>

class __attribute__((__packed__)) InterruptDescriptorTableEntry {
private:
    u16 addressLow;
    u16 codeSegment;
    u8 unused1;
    u8 typeAtts;
    u16 addressMid;
    u32 addressHigh;
    u32 unused2;

public:
    void init(InterruptServiceRoutine *interruptServiceRoutine, bool user);
};

#endif // INTERRUPTDESCRIPTORTABLEENTRY_HPP
