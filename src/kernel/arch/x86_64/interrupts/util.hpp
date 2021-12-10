#pragma once

#include <common/inttypes.hpp>

extern "C" void loadInterruptDescriptorTable(void *idtr);
extern "C" void loadGlobalDescriptorTable(void *gdtr);
extern "C" void loadTaskStateSegment(size_t processorID);
extern "C" uint64_t readCR2();
