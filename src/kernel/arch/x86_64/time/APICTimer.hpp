#pragma once

#include <common/common.hpp>

namespace apic {

class LocalAPIC;

class APICTimer {
private:
    apic::LocalAPIC *apic;

public:
    APICTimer(apic::LocalAPIC *apic) noexcept :
        apic{apic} {}

    void startCounting(uint32_t initialCount) noexcept;
    uint32_t readValue() noexcept;
    void delayMilliseconds(uint32_t ms) noexcept;
};

}
