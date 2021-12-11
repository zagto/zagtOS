#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t ReceiveMessage(const shared_ptr<Process> &process,
                   size_t _portHandles,
                   size_t count,
                   size_t,
                   size_t,
                   size_t);

