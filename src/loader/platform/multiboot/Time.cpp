#include <Time.hpp>
#include <LegacyTimer.hpp>

static LegacyTimer legacyTimer;

uint64_t timerFrequency = 0;

void detectTimerFrequency() {
    /* let the legacy timer wrap after 0xffff ticks */
    legacyTimer.setReloadValue(0xffff);
    legacyTimer.setOperatingMode();

    const uint64_t startValue = readTimerValue();
    const uint64_t numRuns = 50;

    for (uint64_t i = 0; i < numRuns; i++) {
        /* first wait for the value to drop below half */
        while (legacyTimer.readValue() >= 0x8000) {}
        /* now if it's bigger again the timer wrapped around */
        while (legacyTimer.readValue() < 0x8000) {}
    }

    const uint64_t dif = readTimerValue() - startValue;
    const uint64_t legacyFrequency = 1193182;

    timerFrequency = (legacyFrequency * dif) / (numRuns * 0xffff) * 2;

    cout << "TSC timer frequency: " << timerFrequency << "Hz" << endl;
}

void delayMilliseconds(uint64_t ms) {
    assert(timerFrequency != 0);

    uint64_t startValue = readTimerValue();
    uint64_t endValue = startValue + (timerFrequency * ms) / 1000;
    uint64_t now = startValue;
    while (now < endValue) {
        now = readTimerValue();
    }
}
