#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> CreateSharedMemory(const shared_ptr<Process> &process,
                                  size_t type,
                                  size_t offset,
                                  size_t length,
                                  size_t deviceAddressesPointer,
                                  size_t);

