#pragma once

#include <common/inttypes.hpp>

class Thread;
class RegisterState;

bool Futex(Thread *thread, size_t address, uint32_t operation, size_t timeoutOrValue2, int32_t passedValue);
