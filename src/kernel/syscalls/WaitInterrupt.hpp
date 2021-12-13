#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

static constexpr size_t WAIT_INTERRUPT_SUCCESS = 1;
static constexpr size_t WAIT_INTERRUPT_CANCELED = 0;

size_t WaitInterrupt(const shared_ptr<Process> &process,
                             size_t handle,
                             size_t,
                             size_t,
                             size_t,
                             size_t);

