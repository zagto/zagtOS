#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t SubscribeInterrupt(const shared_ptr<Process> &process,
                                  size_t handle,
                                  size_t,
                                  size_t,
                                  size_t,
                                  size_t);

