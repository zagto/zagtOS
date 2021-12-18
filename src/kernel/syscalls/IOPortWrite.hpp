#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t IOPortWrite(const shared_ptr<Process> &process,
                   size_t handle,
                   size_t offset,
                   size_t size,
                   size_t value,
                   size_t);

