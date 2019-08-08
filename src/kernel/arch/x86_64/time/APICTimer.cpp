#include <interrupts/LocalAPIC.hpp>
#include <system/System.hpp>
#include <time/APICTimer.hpp>


void APICTimer::startCounting(uint32_t initialCount) {
    apic->writeRegister(LocalAPIC::Register::DIVIDE_CONFIGURATION, 0);
    apic->writeRegister(LocalAPIC::Register::LVT_TIMER, 0x10000);
    apic->writeRegister(LocalAPIC::Register::INITIAL_COUNT, initialCount);
}


uint32_t APICTimer::readValue() {
    return apic->readRegister(LocalAPIC::Register::CURRENT_COUNT);
}


void APICTimer::delayMilliseconds(uint32_t ms) {
    uint64_t freq = CurrentSystem.time.timerFrequency;
    assert(freq != 0);

    startCounting(static_cast<uint32_t>((freq * static_cast<uint64_t>(ms)) / 1000ul));
    while (readValue() > 0) {}
}
