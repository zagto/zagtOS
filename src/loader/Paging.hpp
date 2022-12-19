#pragma once
#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <setup/HandOverState.hpp>

typedef uint64_t PageTableEntry;
#define ENTRIES_PER_PAGE_TABLE (PAGE_SIZE / sizeof(PageTableEntry))
typedef PageTableEntry PageTable[ENTRIES_PER_PAGE_TABLE];

enum class PagingContext {
    HANDOVER, PROCESS, GLOBAL
};

enum CacheType : size_t {
    CACHE_NORMAL_WRITE_BACK, CACHE_NONE, CACHE_WRITE_THROUGH, CACHE_WRITE_COMBINING
};

void InitPaging();
void ClearPageTable(PageTable *pageTable);
void CreateGlobalMasterPageTableEntries();
void MapAddress(PagingContext pagingContext,
                VirtualAddress virtualAddress,
                PhysicalAddress physicalAddress,
                bool writeable,
                bool executable,
                bool large,
                CacheType cacheType);
hos_v1::PagingContext GetPagingContext();

extern "C" PageTable *GetCurrentMasterPageTable();
