#pragma once

#include <interrupts/RegisterState.hpp>
#include <processes/Process.hpp>

bool GetTime(RegisterState &registerState, const shared_ptr<Process> &process);
