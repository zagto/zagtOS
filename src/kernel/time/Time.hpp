#pragma once

#include <common/common.hpp>
#include <mutex>
#include <optional>

enum ClockID {
    REALTIME = 0, MONOTONIC = 1, COUNT = 2
};

class Time {
public:
    SpinLock offsetLock;
    bool offsetValid{false};
    uint64_t timerFrequency{0};
    uint64_t nanoSecondsToRealTime;

    Time(uint64_t timerFrequency) noexcept;
    void delayMilliseconds(uint64_t ms) const noexcept;
    int64_t getClockValue(size_t clockID) noexcept;
    optional<uint64_t> nanoSecondsToSystemClock(size_t clockID, uint64_t nanoseconds) noexcept;
};

void setTimer(uint64_t value) noexcept;
extern "C" uint64_t readTimerValue();
