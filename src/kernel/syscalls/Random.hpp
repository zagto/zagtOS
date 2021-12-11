#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t Random(const shared_ptr<Process> &process,
                      size_t address,
                      size_t length,
                      size_t,
                      size_t,
                      size_t);

