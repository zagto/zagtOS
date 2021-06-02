#pragma once

#include <common/common.hpp>
#include <lib/Status.hpp>
#include <mutex>

namespace kernelPageAllocator {

class Allocator {
private:
    static constexpr size_t TOTAL_NUM_HEAP_FRAMES = KernelHeapRegion.length / PAGE_SIZE;
    static constexpr size_t FRAMES_PER_GROUP = PLATFORM_BITS;
    static constexpr size_t NUM_BITMAP_GROUPS = TOTAL_NUM_HEAP_FRAMES / FRAMES_PER_GROUP;
    static constexpr size_t ALL_BITS = static_cast<size_t>(-1);

    size_t bitmap[NUM_BITMAP_GROUPS]{0};
    size_t startPosition = 0;

    static_assert(KernelHeapRegion.length % PAGE_SIZE == 0);
    static_assert(TOTAL_NUM_HEAP_FRAMES % FRAMES_PER_GROUP == 0);

    bool getFrame(size_t frame) const;
    void setFrame(size_t frame);
    void unsetFrame(size_t frame);

public:
    void unmap(void *_address, size_t length, bool freeFrames);
    Result<void *> map(size_t length, bool findNewFrames, const PhysicalAddress *frames = nullptr);

    SpinLock lock;
};

extern Allocator KernelPageAllocator;
}

using kernelPageAllocator::KernelPageAllocator;
