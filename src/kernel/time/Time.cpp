#include <system/System.hpp>
#include <time/Time.hpp>
#include <time/Time.hpp>
#include <common/ModelSpecificRegister.hpp>

Time::Time(uint64_t timerFrequency):
    timerFrequency{timerFrequency} {
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

void setTimer(uint64_t value) {
    writeModelSpecificRegister(MSR::TSC_DEADLINE, value);
}
