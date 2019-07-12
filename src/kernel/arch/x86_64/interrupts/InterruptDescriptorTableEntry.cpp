#include <interrupts/InterruptDescriptorTableEntry.hpp>


void InterruptDescriptorTableEntry::init(InterruptServiceRoutine *interruptServiceRoutine, bool user) {
    usize address = reinterpret_cast<usize>(interruptServiceRoutine);

    // Address 0-15
    addressLow = address;

    codeSegment = 0x8;
    unused1 = 0;

    // Type/attributes
    // 1    - Present: yes
    // 00   - Privilege Level: Ring 0
    // 0    - Should be 0 fer interrupt gates
    // 1110 - Type: i386 interrupt gate
    typeAtts = 0b10001110;
    if (user) {
        typeAtts |= 0b01100000;
    }

    // Address 13-31
    addressMid = address >> 16;

    // Address 32-63
    addressHigh = address >> 32;

    unused2 = 0;
}
