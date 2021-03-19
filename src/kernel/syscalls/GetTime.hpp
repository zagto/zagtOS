#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> GetTime(const shared_ptr<Process> &process,
                       size_t clockID,
                       size_t resultAddress,
                       size_t,
                       size_t,
                       size_t);

