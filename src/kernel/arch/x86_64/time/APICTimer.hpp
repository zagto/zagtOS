#pragma once

#include <common/common.hpp>

class LocalAPIC;

class APICTimer {
private:
    LocalAPIC *apic;

public:
    APICTimer(LocalAPIC *apic) noexcept :
        apic{apic} {}

    void startCounting(uint32_t initialCount) noexcept;
    uint32_t readValue() noexcept;
    void delayMilliseconds(uint32_t ms) noexcept;
};
