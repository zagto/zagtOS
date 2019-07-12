#ifndef INTERRUPTSERIVCEROUTINE_HPP
#define INTERRUPTSERIVCEROUTINE_HPP

#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>

class InterruptServiceRoutine {
    // dummy class, 12 bytes big
private:
    u8 dummy[12];
};

static const usize NUM_INTERRUPT_SERIVICE_ROUTINES = 256;
extern "C" InterruptServiceRoutine InterruptServiceRoutines[NUM_INTERRUPT_SERIVICE_ROUTINES];

extern "C" __attribute__((noreturn)) void returnFromInterrupt(RegisterState *registerState,
                                                              UserVirtualAddress fsBase);

#endif // INTERRUPTSERIVCEROUTINE_HPP
