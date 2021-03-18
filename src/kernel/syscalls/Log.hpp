#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> Log(const shared_ptr<Process> &process,
           uint64_t _address,
           uint64_t _length,
           uint64_t,
           uint64_t,
           uint64_t);
