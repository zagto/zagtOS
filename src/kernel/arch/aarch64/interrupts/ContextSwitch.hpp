#pragma once

#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>

extern "C" [[noreturn]] void returnFromInterrupt(RegisterState *registerState);
extern "C" char syscallEntry;
