#include <system/System.hpp>
#include <time/Time.hpp>
#include <time/Time.hpp>

void Time::initialize() {
    detectTimerFrequency();
}

void Time::detectTimerFrequency() {
    /* on x86_64, waking secondary processors requires timing, so it should always happen after
     * this */
    assert(CurrentSystem.processors.size() == 1);


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

void Time::delayMilliseconds(uint64_t ms) {
    assert(timerFrequency != 0);

    uint64_t startValue = readTimerValue();
    uint64_t endValue = startValue + (timerFrequency * ms) / 1000;
    uint64_t now = startValue;
    while (now < endValue) {
        now = readTimerValue();
    }
}
