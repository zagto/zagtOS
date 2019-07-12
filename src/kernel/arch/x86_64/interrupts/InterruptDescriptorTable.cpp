#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/Interrupts.hpp>


InterruptDescriptorTable::InterruptDescriptorTable() {
    for (usize index = 0; index < NUM_INTERRUPT_SERIVICE_ROUTINES; index++) {
        entries[index].init(&InterruptServiceRoutines[index],
                            index == Interrupts::SYSCALL_INTERRUPT);
    }
}
