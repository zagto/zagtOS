#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> ReceiveMessage(const shared_ptr<Process> &process,
                   size_t portHandle,
                   size_t,
                   size_t,
                   size_t,
                   size_t);

