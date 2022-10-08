#pragma once
#include <common/addresses.hpp>

void *operator new(size_t, void *address);
void *operator new(size_t, KernelVirtualAddress address);
