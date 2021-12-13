#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t CreateInterrupt(const shared_ptr<Process> &process,
                       size_t type,
                       size_t typeData,
                       size_t triggerMode,
                       size_t polarity,
                       size_t);

