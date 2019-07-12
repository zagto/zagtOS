#ifndef VIRTUALMEMORY_H
#define VIRTUALMEMORY_H

#include <efi.h>

struct VirtualMemoryRegion {
    UINTN start;
    UINTN end;
};

static const struct VirtualMemoryRegion KernelImageRegion = {
    .start = 0xffff800000000000,
    .end = 0xffff8fffffffffff,
};

static const struct VirtualMemoryRegion FramebufferRegion = {
    .start = 0xffff900000000000,
    .end = 0xffff9fffffffffff,
};

static const struct VirtualMemoryRegion KernelPersistentDataRegion = {
    .start = 0xffffa00000000000,
    .end = 0xffffa0003fffffff,
};

static const UINTN IDENTITY_MAPPING_BASE = 0xffffb00000000000;

static const EFI_VIRTUAL_ADDRESS SystemInfo =            0xffffa00000000000;


void InitVirtualMemory();


#endif // VIRTUALMEMORY_H
