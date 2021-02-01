#include <interrupts/InterruptDescriptorTableEntry.hpp>


void InterruptDescriptorTableEntry::init(InterruptServiceRoutine *interruptServiceRoutine, uint8_t ISTOffset) {
    size_t address = reinterpret_cast<size_t>(interruptServiceRoutine);

    // Address 0-15
    addressLow = address;

    codeSegment = 0x8;
    this->ISTOffset = ISTOffset;

    // Type/attributes
    // 1    - Present: yes
    // 00   - Privilege Level: Ring 0
    // 0    - Should be 0 fer interrupt gates
    // 1110 - Type: i386 interrupt gate
    typeAtts = 0b10001110;

    // Address 13-31
    addressMid = address >> 16;

    // Address 32-63
    addressHigh = address >> 32;

    unused2 = 0;
}
