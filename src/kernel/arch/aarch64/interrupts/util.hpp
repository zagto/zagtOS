#pragma once

#include <common/inttypes.hpp>

extern "C" void SetUserTLSRegister(uint64_t value);
extern "C" void SetExceptionVectorTable(const void *address);
extern "C" uint64_t readFAR();
