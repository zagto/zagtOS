#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t MMap(const shared_ptr<Process> &process,
                    size_t structAddress,
                    size_t,
                    size_t,
                    size_t,
                    size_t);

