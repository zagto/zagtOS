#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

Result<size_t> MUnmap(const shared_ptr<Process> &process,
                      size_t wholeArea,
                      size_t startAddress,
                      size_t length,
                      size_t,
                      size_t);

