#pragma once

#include <common/common.hpp>

extern "C" uint64_t readTimerValue();

extern uint64_t timerFrequency;
void detectTimerFrequency();
void delayMilliseconds(uint64_t ms);
