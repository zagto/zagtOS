#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> IOPortWrite(const shared_ptr<Process> &process,
                           size_t port,
                           size_t size,
                           size_t value,
                           size_t,
                           size_t);

