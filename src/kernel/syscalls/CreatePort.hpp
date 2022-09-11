#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t CreatePort(const shared_ptr<Process> &process,
                  size_t eventQueueHandle,
                  size_t,
                  size_t,
                  size_t,
                  size_t);
