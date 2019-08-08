#ifndef MODELSPECIFICREGISTER_HPP
#define MODELSPECIFICREGISTER_HPP

#include <common/common.hpp>

extern "C" uint64_t readModelSpecificRegister(uint32_t id);
extern "C" void writeModelSpecificRegister(uint32_t id, uint64_t value);

#endif // MODELSPECIFICREGISTER_HPP
