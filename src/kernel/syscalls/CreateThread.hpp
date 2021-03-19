#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> CreateThread(const shared_ptr<Process> &process,
                            size_t entry,
                            size_t stack,
                            size_t priority,
                            size_t tls,
                            size_t);

