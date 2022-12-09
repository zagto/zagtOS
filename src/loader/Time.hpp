#pragma once

#include <common/common.hpp>

extern "C" uint64_t readTimerValue();

extern uint64_t TimerFrequency;
extern "C" void detectTimerFrequency();
extern "C" void delayMilliseconds(uint64_t ms);
