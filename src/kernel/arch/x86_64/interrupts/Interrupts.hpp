#ifndef INTERRUPTS_HPP
#define INTERRUPTS_HPP

#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/Record.hpp>
#include <interrupts/RegisterState.hpp>

class Interrupts {
public:
    static const size_t SYSCALL_INTERRUPT = 0xff;

private:
    GlobalDescriptorTable globalDescriptorTable;
    Record<GlobalDescriptorTable> globalDescriptorTableRecord;
    Record<InterruptDescriptorTable> interruptDescriptorTableRecord;
    TaskStateSegment taskStateSegment;

public:
    Interrupts();

    __attribute__((noreturn)) void handler(RegisterState *registerState);
    __attribute__((noreturn)) void userHandler(RegisterState *registerState);
    __attribute__((noreturn)) void kernelHandler(RegisterState *registerState);
    __attribute__((noreturn)) void returnToUserMode();
};

extern InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;

#endif // INTERRUPTS_HPP
