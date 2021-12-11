#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t Exit(const shared_ptr<Process> &process,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t);
