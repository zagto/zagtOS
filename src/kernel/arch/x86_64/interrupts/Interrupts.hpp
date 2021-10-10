#pragma once

#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/Record.hpp>
#include <interrupts/RegisterState.hpp>
#include <interrupts/LocalAPIC.hpp>

enum class StaticInterrupt : uint8_t {
    PAGE_FAULT = 0x0e,
    PIC1_SPURIOUS = 0x20,
    PIC2_SPURIOUS = 0x21,
    APIC_SPURIOUS = 0x22,
    IPI = 0x31,
};

static inline bool operator==(uint64_t a, StaticInterrupt b) {
    return a == static_cast<uint64_t>(b);
}

static Region X86ExceptionRegion{0x00, 0x20};

extern "C" void basicEnableInterrupts();
extern "C" void basicDisableInterrupts();

[[noreturn]] void _handleInterrupt(RegisterState* registerState);
void dealWithException(Status status);

extern InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;
