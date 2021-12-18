#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t CreateIOPortRange(const shared_ptr<Process> &process,
                                 size_t start,
                                 size_t length,
                                 size_t,
                                 size_t,
                                 size_t);

