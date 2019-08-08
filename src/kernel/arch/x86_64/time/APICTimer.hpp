#ifndef APICTIMER_HPP
#define APICTIMER_HPP

#include <common/common.hpp>

class LocalAPIC;

class APICTimer {
private:
    LocalAPIC *apic;

public:
    APICTimer(LocalAPIC *apic) :
        apic{apic} {}

    void startCounting(uint32_t initialCount);
    uint32_t readValue();
    void delayMilliseconds(uint32_t ms);
};

#endif // APICTIMER_HPP
