#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/Interrupts.hpp>


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
        entries[index].init(&InterruptServiceRoutines[index], ISTEntry);
    }
}
