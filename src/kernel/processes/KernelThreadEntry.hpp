#pragma once
#include <common/common.hpp>

void IdleThreadEntry(void *);
void UserReturnEntry(void *);
extern "C" void InKernelReturnEntry(void *);
extern "C" void InKernelReturnEntryRestoreInterruptsLock(RegisterState *);
