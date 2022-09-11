#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t SubscribeInterrupt(const shared_ptr<Process> &process,
                                  size_t BoundInterruptHandle,
                                  size_t eventQueueHandle,
                                  size_t eventTag,
                                  size_t,
                                  size_t);

