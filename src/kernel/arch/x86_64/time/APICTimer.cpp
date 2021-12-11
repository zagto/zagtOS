#include <interrupts/LocalAPIC.hpp>
#include <system/System.hpp>
#include <time/APICTimer.hpp>


void APICTimer::startCounting(uint32_t initialCount) noexcept {
    apic->writeRegister(LocalAPIC::Register::DIVIDE_CONFIGURATION, 0);
    apic->writeRegister(LocalAPIC::Register::LVT_TIMER, 0x10000);
    apic->writeRegister(LocalAPIC::Register::INITIAL_COUNT, initialCount);
}


uint32_t APICTimer::readValue() noexcept {
    return apic->readRegister(LocalAPIC::Register::CURRENT_COUNT);
}


void APICTimer::delayMilliseconds(uint32_t ms) noexcept {
    uint64_t freq = CurrentSystem.time.timerFrequency;
    assert(freq != 0);

    startCounting(static_cast<uint32_t>((freq * static_cast<uint64_t>(ms)) / 1000ul));
    while (readValue() > 0) {}
}
