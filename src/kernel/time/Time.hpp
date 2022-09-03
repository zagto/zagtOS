#pragma once

#include <common/common.hpp>
#include <mutex>

enum ClockID {
    REALTIME, MONOTONIC, COUNT
};

class Time {
public:
    mutex offsetLock;
    bool offsetValid{false};
    uint64_t timerFrequency{0};
    timespec realTimeOffset;

    Time(uint64_t timerFrequency);
    void delayMilliseconds(uint64_t ms);
};

void setTimer(uint64_t value);
extern "C" uint64_t readTimerValue();
