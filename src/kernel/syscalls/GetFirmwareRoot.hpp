#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t GetFirmwareRoot(const shared_ptr<Process> &,
                               size_t,
                               size_t,
                               size_t,
                               size_t,
                               size_t);

