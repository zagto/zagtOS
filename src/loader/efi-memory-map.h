#ifndef EFI_MEMORY_MAP_H
#define EFI_MEMORY_MAP_H

#include <efi.h>

struct EfiMemoryMapInfo {
    EFI_MEMORY_DESCRIPTOR *map;
    UINTN descriptorSize;
    UINTN descriptorVersion;
    UINTN numDescriptors;
    UINTN currentIndex; // used to iterate over map
};


void GetMemoryMapAndExitBootServices(EFI_HANDLE imageHandle, struct EfiMemoryMapInfo *mapInfo);

EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetFirstAvailableEntry(struct EfiMemoryMapInfo *mapInfo);
EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetNextAvailableEntry(struct EfiMemoryMapInfo *mapInfo);

EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetFirstReclaimableEntry(struct EfiMemoryMapInfo *mapInfo);
EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetNextReclaimableEntry(struct EfiMemoryMapInfo *mapInfo);

#endif // EFI_MEMORY_MAP_H
