#ifndef UTIL_HPP
#define UTIL_HPP

#include <interrupts/Record.hpp>

extern "C" void loadInterruptDescriptorTable(Record<InterruptDescriptorTable> *idtr);
extern "C" void loadGlobalDescriptorTable(Record<GlobalDescriptorTable> *gdtr);
extern "C" void loadTaskStateSegment();
extern "C" uint64_t readCR2();
extern "C" void setFSBase(UserVirtualAddress value);
extern "C" char idleEntry;

#endif // UTIL_HPP
