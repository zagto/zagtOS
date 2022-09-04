#include <system/System.hpp>
#include <time/Time.hpp>
#include <time/Time.hpp>
#include <common/ModelSpecificRegister.hpp>

Time::Time(uint64_t timerFrequency) noexcept:
    timerFrequency{timerFrequency} {
}

int64_t Time::getClockValue(size_t clockID) noexcept {
    assert(clockID < ClockID::COUNT);
    __uint128_t value = readTimerValue();
    /* we want nanoseconds instead of seconds */
    value *= 1'000'000'000;
    value /= timerFrequency;

    if (clockID == ClockID::REALTIME) {
        scoped_lock sl1(KernelInterruptsLock);
        scoped_lock sl2(offsetLock);
        if (offsetValid) {
            value += nanoSecondsToRealTime;
        } else {
            value = 0;
        }
    }

    /* system would need to run really long for time that cannot be counted as nanoseconds in a
     * 64-bit integer */
    assert(value == static_cast<uint64_t>(value));
    return static_cast<uint64_t>(value);
}

optional<uint64_t> Time::nanoSecondsToSystemClock (
        size_t clockID,
        uint64_t nanoseconds) noexcept {
    assert(clockID < ClockID::COUNT);
    __uint128_t value = nanoseconds;

    if (clockID == ClockID::REALTIME) {
        scoped_lock sl1(KernelInterruptsLock);
        scoped_lock sl2(offsetLock);
        if (!offsetValid) {
            cout << "nanoSecondsToSystemClock: real time no initialized" << endl;
            return {};
        }
        if (value >= nanoSecondsToRealTime) {
            value -= nanoSecondsToRealTime;
        } else {
            cout << "nanoSecondsToSystemClock: realtime value before system booted" << endl;
            value = 0;
        }
    }

    value *= timerFrequency;
    /* value was in nanoseconds instead of seconds */
    value /= 1'000'000'000;

    if (value == static_cast<uint64_t>(value)) {
        return {static_cast<uint64_t>(value)};
    } else {
        cout << "nanoSecondsToSystemClock: result value would overflow" << endl;
        return {};
    }
}


void Time::delayMilliseconds(uint64_t ms) const noexcept {
    assert(timerFrequency != 0);

    uint64_t startValue = readTimerValue();
    uint64_t endValue = startValue + (timerFrequency * ms) / 1000;
    uint64_t now = startValue;
    while (now < endValue) {
        now = readTimerValue();
    }
}

void setTimer(uint64_t value) noexcept {
    writeModelSpecificRegister(MSR::TSC_DEADLINE, value);
}
