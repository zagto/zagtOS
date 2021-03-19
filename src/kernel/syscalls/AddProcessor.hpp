#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> AddProcessor(const shared_ptr<Process> &process,
                            size_t hardwareID,
                            size_t,
                            size_t,
                            size_t,
                            size_t);

