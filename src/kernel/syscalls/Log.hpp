#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t Log(const shared_ptr<Process> &process,
           size_t _address,
           size_t _length,
           size_t,
           size_t,
           size_t);
