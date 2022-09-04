#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t NanoSleep(const shared_ptr<Process> &process,
                 size_t flags,
                 size_t clockID,
                 size_t requestedTimespecAddress,
                 size_t,
                 size_t);

