#ifndef __ZAGTOS_SHARED_MEMORY_H
#define __ZAGTOS_SHARED_MEMORY_H

#include <zagtos/HandleObject.h>
#include <stddef.h>

int ZoCreateDMASharedMemory(size_t deviceMax, size_t length, size_t *deviceAddresses);
int ZoCreatePhysicalSharedMemory(size_t physicalAddress, size_t length);
int ZoCreateStandardSharedMemory(size_t length);
void ZoUnmapWhole(void *pointer);

#endif
