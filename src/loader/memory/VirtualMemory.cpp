#include <memory/VirtualMemory.hpp>
#include <Paging.hpp>
#include <iostream>
#include <memory/ArchRegions.hpp>
#include <MemoryMap.hpp>
#include <memory/PhysicalMemory.hpp>

extern char _data;

void MapLoaderMemory() {
    const size_t dataAddress = reinterpret_cast<size_t>(&_data);
    assert(dataAddress % PAGE_SIZE == 0);

    for (optional<Region> region = memoryMap::firstReclaimableRegion();
         region;
         region = memoryMap::nextReclaimableRegion()) {
        for (size_t frameIndex = 0; frameIndex < region->length / PAGE_SIZE; frameIndex++) {
            bool isData = region->start + frameIndex * PAGE_SIZE >= dataAddress;
            MapAddress(PagingContext::HANDOVER,
                       region->start + frameIndex * PAGE_SIZE,
                       region->start + frameIndex * PAGE_SIZE,
                       isData,
                       !isData,
                       false,
                       CacheType::CACHE_NORMAL_WRITE_BACK);
        }
    }
}


void MapFramebufferMemory(hos_v1::FramebufferInfo &framebufferInfo) {
    if (framebufferInfo.type == hos_v1::FramebufferType::NO_FRAMEBUFFER) {
        return;
    }
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

void MapSerialMemory(hos_v1::SerialInfo &serialInfo) {
    if (serialInfo.type == hos_v1::NO_SERIAL || serialInfo.memoryLength == 0) {
        return;
    }
    size_t baseAddress = serialInfo.baseAddress;
    size_t serialOffset = baseAddress % PAGE_SIZE;
    size_t firstAddress = baseAddress - serialOffset;
    size_t lastAddress = baseAddress + serialInfo.memoryLength;
    size_t numPages = (lastAddress - firstAddress - 1) / PAGE_SIZE + 1;
    size_t index;

    /* mapping for kernel */
    for (index = 0; index < numPages; index++) {
        MapAddress(PagingContext::GLOBAL,
                   SerialRegion.start + index * PAGE_SIZE,
                   firstAddress + index * PAGE_SIZE,
                   true,
                   false,
                   false,
                   CacheType::CACHE_NONE);
    }
    /* mapping for loader */
    for (index = 0; index < numPages; index++) {
        MapAddress(PagingContext::HANDOVER,
                   firstAddress + index * PAGE_SIZE,
                   firstAddress + index * PAGE_SIZE,
                   true,
                   false,
                   false,
                   CacheType::CACHE_NONE);
    }

    /* update serial info to contain the "new" address, as this structure will be passed to the
     * kernel later on */
    serialInfo.baseAddress = SerialRegion.start + serialOffset;
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
