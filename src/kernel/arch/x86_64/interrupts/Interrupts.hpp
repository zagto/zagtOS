#ifndef INTERRUPTS_HPP
#define INTERRUPTS_HPP

#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/Record.hpp>
#include <interrupts/RegisterState.hpp>
#include <interrupts/LegacyPIC.hpp>
#include <interrupts/LocalAPIC.hpp>

class Interrupts {
public:
    static const size_t SYSCALL_INTERRUPT = 0xff;
    static const size_t PIC1_SPURIOUS_IRQ = 0x20;
    static const size_t PIC2_SPURIOUS_IRQ = 0x21;

private:
    GlobalDescriptorTable globalDescriptorTable;
    Record<GlobalDescriptorTable> globalDescriptorTableRecord;
    Record<InterruptDescriptorTable> interruptDescriptorTableRecord;
    TaskStateSegment taskStateSegment;
    LegacyPIC legacyPIC;

    __attribute__((noreturn)) void userHandler(RegisterState *registerState);
    __attribute__((noreturn)) void kernelHandler(RegisterState *registerState);

protected:
    friend class Time;
    LocalAPIC localAPIC;

public:
    Interrupts(bool bootProcessor);

    __attribute__((noreturn)) void handler(RegisterState *registerState);
    __attribute__((noreturn)) void returnToUserMode();
    void wakeSecondaryProcessor(size_t hardwareID);
};

extern InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;

#endif // INTERRUPTS_HPP
