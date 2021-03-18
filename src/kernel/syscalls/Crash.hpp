#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> Crash(const shared_ptr<Process> &process,
            uint64_t,
            uint64_t,
            uint64_t,
            uint64_t,
            uint64_t);
