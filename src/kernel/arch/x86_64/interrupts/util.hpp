#ifndef UTIL_HPP
#define UTIL_HPP

#include <interrupts/Record.hpp>

extern "C" void loadInterruptDescriptorTable(void *idtr);
extern "C" void loadGlobalDescriptorTable(void *gdtr);
extern "C" void loadTaskStateSegment(size_t processorID);
extern "C" uint64_t readCR2();
extern "C" void setFSBase(UserVirtualAddress value);
extern "C" char idleEntry;

#endif // UTIL_HPP
