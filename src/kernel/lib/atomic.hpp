#pragma once

#include <common/inttypes.hpp>

extern "C"
bool compare_exchange_i32(volatile int32_t &variable, int32_t &expected, int32_t desired);
bool compare_exchange_u32(volatile uint32_t &variable, uint32_t &expected, uint32_t desired);
