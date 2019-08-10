#ifndef PAGING_H
#define PAGING_H

#include <efi.h>
#include <util.h>
#include <efi-memory-map.h>
#include <framebuffer.h>

typedef UINT64 PageTableEntry;
#define ENTRIES_PER_PAGE_TABLE (PAGE_SIZE / sizeof(PageTableEntry))
typedef PageTableEntry PageTable[ENTRIES_PER_PAGE_TABLE];

extern PageTable *MasterPageTable;

void InitPaging(void);
void MapLoaderMemory(struct EfiMemoryMapInfo *mapInfo);
void MapFramebufferMemory(struct FramebufferInfo *framebufferInfo);
void CreateIdentityMap(EFI_PHYSICAL_ADDRESS maxPhysicalAddress);

void MapAddress(EFI_PHYSICAL_ADDRESS physicalAddress,
                EFI_VIRTUAL_ADDRESS virtualAddress,
                BOOLEAN writeable,
                BOOLEAN executable,
                BOOLEAN large);

#endif // VIRTUAL_MEMORY_H
