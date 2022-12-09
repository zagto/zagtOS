#pragma once
#include <common/Region.hpp>

enum class InterruptType : uint8_t {
    X86_GSI, PROCESSOR_DIRECT
};
static constexpr Region DynamicInterruptRegion{0, 0xff};

inline void basicEnableInterrupts() {}
inline void basicDisableInterrupts() {}
