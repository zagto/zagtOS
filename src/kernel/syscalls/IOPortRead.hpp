#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t IOPortRead(const shared_ptr<Process> &process,
                          size_t port,
                          size_t size,
                          size_t,
                          size_t,
                          size_t);

