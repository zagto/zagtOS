#include <memory/VirtualMemory.hpp>
#include <Paging.hpp>
#include <iostream>
#include <memory/ArchRegions.hpp>
#include <MemoryMap.hpp>
#include <memory/PhysicalMemory.hpp>


void InitPaging(void) {
    cout << "Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;
    ClearPageTable(HandOverMasterPageTable);
    CreateGlobalMasterPageTableEntries();
}


void MapLoaderMemory() {
    size_t frameIndex;

    for (optional<Region> region = memoryMap::firstReclaimableRegion();
         region;
         region = memoryMap::nextReclaimableRegion()) {
        for (frameIndex = 0; frameIndex < region->length / PAGE_SIZE; frameIndex++) {
            MapAddress(PagingContext::HANDOVER,
                       region->start + frameIndex * PAGE_SIZE,
                       region->start + frameIndex * PAGE_SIZE,
                       true,
                       true,
                       false,
                       CacheType::CACHE_NORMAL_WRITE_BACK);
        }
    }
}


void MapFramebufferMemory(hos_v1::FramebufferInfo &framebufferInfo) {
    size_t frontBufferAddress = reinterpret_cast<size_t>(framebufferInfo.frontBuffer);
    size_t framebufferOffset = frontBufferAddress % PAGE_SIZE;
    size_t firstAddress = frontBufferAddress - framebufferOffset;
    size_t lastAddress = frontBufferAddress + framebufferInfo.height * framebufferInfo.bytesPerLine;
    size_t numPages = (lastAddress - firstAddress - 1) / PAGE_SIZE + 1;
    size_t index;

    /* front buffer for kernel */
    for (index = 0; index < numPages; index++) {
        MapAddress(PagingContext::GLOBAL,
                   FramebufferRegion.start + index * PAGE_SIZE,
                   firstAddress + index * PAGE_SIZE,
                   true,
                   false,
                   false,
                   CacheType::CACHE_WRITE_COMBINING);
    }
    /* front buffer for loader */
    for (index = 0; index < numPages; index++) {
        MapAddress(PagingContext::HANDOVER,
                   firstAddress + index * PAGE_SIZE,
                   firstAddress + index * PAGE_SIZE,
                   true,
                   false,
                   false,
                   CacheType::CACHE_WRITE_COMBINING);
    }

    /* back buffer for kernel */
    for (index = numPages; index < numPages * 2; index++) {
        MapAddress(PagingContext::GLOBAL,
                   FramebufferRegion.start + index * PAGE_SIZE,
                   AllocatePhysicalFrame(),
                   true,
                   false,
                   false,
                   CacheType::CACHE_NORMAL_WRITE_BACK);
    }

    /* update framebuffer info to contain the "new" address, as this structure will be passed to the
     * kernel later on */
    framebufferInfo.frontBuffer =
            reinterpret_cast<uint8_t *>(FramebufferRegion.start + framebufferOffset);
    framebufferInfo.backBuffer = framebufferInfo.frontBuffer + numPages * PAGE_SIZE;
}


void CreateIdentityMap(PhysicalAddress maxPhysicalAddress) {
    PhysicalAddress addr;
    for (addr = 0;
         addr.value() < maxPhysicalAddress.value();
         addr = addr.value() + 2 * 1024 * 1024) { /* 2MiB */

        MapAddress(PagingContext::GLOBAL,
                   addr.value() + IdentityMapping.start,
                   addr,
                   true,
                   false,
                   true,
                   CacheType::CACHE_NORMAL_WRITE_BACK);
    }
}
