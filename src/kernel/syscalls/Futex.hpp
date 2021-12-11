#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t Futex(const shared_ptr<Process> &,
                     size_t address,
                     size_t operation,
                     size_t timeoutOrValue2,
                     size_t passedValue,
                     size_t);
