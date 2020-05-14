#pragma once

#include <common/common.hpp>
#include <time/LegacyTimer.hpp>
#include <mutex>

class Time {
private:
    LegacyTimer legacyTimer;

    void detectTimerFrequency();

public:
    mutex offsetLock;
    bool offsetValid{false};
    uint64_t timerFrequency{0};
    timespec realTimeOffset;

    void initialize();
    void delayMilliseconds(uint64_t ms);
};

extern "C" uint64_t readTimerValue();
