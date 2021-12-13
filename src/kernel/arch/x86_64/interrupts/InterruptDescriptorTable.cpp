#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/util.hpp>


InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;

InterruptDescriptorTable::InterruptDescriptorTable() {
    for (size_t index = 0; index < NUM_INTERRUPT_SERIVICE_ROUTINES; index++) {
        uint8_t ISTEntry = 0;
        if (index == 0x02) {
            /* NMI */
            ISTEntry = 1;
        } else if (index == 0x12) {
            /* MC */
            ISTEntry = 2;
        }
        idt[index].init(&InterruptServiceRoutines[index], ISTEntry);
    }

    idtr.address = idt;
    idtr.size = sizeof(idt) - 1;
}

void InterruptDescriptorTable::load() {
    loadInterruptDescriptorTable(&idtr);
}
