#pragma once

#include <common/common.hpp>
#include <mutex>
#include <paging/PagingContext.hpp>

namespace kernelPageAllocator {

/* TODO: Not very efficient. We should a structure similar to FrameStack, that does not need to
 * does not split information around the individual pages */
class InvalidateQueue {
private:
    struct Item {
        KernelVirtualAddress next;
        size_t referenceCount;
        bool freeFrame;
    };

    KernelVirtualAddress head;

public:
    void add(size_t frameIndex, bool freeFrame) noexcept;
    void localProcessing() noexcept;
};

class Allocator {
private:
    static constexpr size_t TOTAL_NUM_HEAP_FRAMES = KernelHeapRegion.length / PAGE_SIZE;
    static constexpr size_t FRAMES_PER_GROUP = PLATFORM_BITS;
    static constexpr size_t NUM_BITMAP_GROUPS = TOTAL_NUM_HEAP_FRAMES / FRAMES_PER_GROUP;
    static constexpr size_t ALL_BITS = static_cast<size_t>(-1);

    size_t bitmap[NUM_BITMAP_GROUPS]{0};
    size_t startPosition = 0;

    InvalidateQueue invalidateQueue;

    static_assert(KernelHeapRegion.length % PAGE_SIZE == 0);
    static_assert(TOTAL_NUM_HEAP_FRAMES % FRAMES_PER_GROUP == 0);

    bool getFrame(size_t frame) const noexcept;
    void setFrame(size_t frame) noexcept;

    friend class InvalidateQueue;
    void unsetFrame(size_t frame) noexcept;

public:
    void unmap(void *_address, size_t length, bool freeFrames) noexcept;
    void *map(size_t length,
              bool findNewFrames,
              const PhysicalAddress *frames = nullptr,
              CacheType cacheType = CacheType::NORMAL_WRITE_BACK);
    void processInvalidateQueue();

    SpinLock lock;
};

extern Allocator KernelPageAllocator;
}

using kernelPageAllocator::KernelPageAllocator;
