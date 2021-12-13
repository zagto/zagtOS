#pragma once

#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/RegisterState.hpp>

class RegisterState;

/* These are defined to be the same values as for the x86 APIC. If you change the enum, you have
 * to add conversion code there */
enum class TriggerMode : uint8_t {
    EDGE, LEVEL
};
enum class Polarity : uint8_t {
    ACRIVE_HIGH, ACTIVE_LOW
};
enum class InterruptType : uint8_t {
    X86_GSI, PROCESSOR_DIRECT
};

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

static constexpr Region X86ExceptionRegion{0x00, 0x20};
static constexpr Region DynamicInterruptRegion{0x32, 0xFE + 1 - 0x32};

extern "C" void basicEnableInterrupts();
extern "C" void basicDisableInterrupts();

[[noreturn]] void _handleInterrupt(RegisterState* registerState);
void dealWithException(Status status);
