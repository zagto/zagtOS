#pragma once

#include <interrupts/RegisterState.hpp>
#include <processes/Process.hpp>

void GetTime(const shared_ptr<Process> &process, uint32_t clockID, size_t resultAddress);
