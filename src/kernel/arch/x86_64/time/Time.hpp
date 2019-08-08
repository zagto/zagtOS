#ifndef TIME_HPP
#define TIME_HPP

#include <common/common.hpp>
#include <time/LegacyTimer.hpp>

class Time {
    LegacyTimer legacyTimer;

    void detectTimerFrequency();

public:
    uint64_t timerFrequency{0};

    void initialize();
};

#endif // TIME_HPP
