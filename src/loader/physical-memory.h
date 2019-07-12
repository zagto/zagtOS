#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#include <efi.h>
#include <efi-memory-map.h>
#include <framestack.h>
#include <bootinfo.h>

extern struct FrameStack DirtyFrameStack;
extern struct FrameStack CleanFrameStack;

EFI_PHYSICAL_ADDRESS InitPhysicalFrameManagement(struct EfiMemoryMapInfo *mapInfo,
                                                 struct InitDataInfo *initDataInfo);
void *AllocatePhysicalFrame(void);
void DeallocatePhysicalFrame(void *frame);
void ClearFrame(void *frame);

#endif // PHYSICAL_MEMORY_H
