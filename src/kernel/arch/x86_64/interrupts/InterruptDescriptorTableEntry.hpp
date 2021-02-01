#pragma once

#include <common/common.hpp>
#include <interrupts/ContextSwitch.hpp>

class __attribute__((__packed__)) InterruptDescriptorTableEntry {
private:
    uint16_t addressLow;
    uint16_t codeSegment;
    uint8_t ISTOffset;
    uint8_t typeAtts;
    uint16_t addressMid;
    uint32_t addressHigh;
    uint32_t unused2;

public:
    void init(InterruptServiceRoutine *interruptServiceRoutine, uint8_t ISTOffset);
};
