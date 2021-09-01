#pragma once

#include <common/common.hpp>

enum class MSR : uint32_t {
    STAR = 0xC0000081,
    LSTAR = 0xC0000082,
    SFMASK = 0xC0000084,
    IA32_APIC_BASE = 0x1B,
    PLATFORM_ID = 0x17,
    FSB_FREQ = 0xce,
    PLATFORM_INFO = 0xce,
};

extern "C" uint64_t readModelSpecificRegister(MSR msr);
extern "C" void writeModelSpecificRegister(MSR msr, uint64_t value);
