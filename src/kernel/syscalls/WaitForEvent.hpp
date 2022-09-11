#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t WaitForEvent(const shared_ptr<Process> &process,
                   size_t queueHandle,
                   size_t count,
                   size_t,
                   size_t,
                   size_t);

