#include <common/common.hpp>
#include <memory/FrameManagement.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <memory/PageOutContext.hpp>
#include <paging/PagingContext.hpp>
#include <system/Processor.hpp>
#include <lib/Exception.hpp>


namespace kernelPageAllocator {

void Allocator::unmap(void *_address, size_t length, bool freeFrames) noexcept {
    assert(lock.isLocked());
    size_t address = KernelVirtualAddress(_address).value();
    Region region(address, length);

    assert(KernelHeapRegion.contains(region));
    assert(region.isPageAligned());

    size_t firstFrame = (address - KernelHeapRegion.start) / PAGE_SIZE;
    size_t numFrames = length / PAGE_SIZE;

    /*size_t firstGroup = firstFrame / PLATFORM_BITS;
    size_t lastGroup = (firstFrame + numFrames - 1) / PLATFORM_BITS;
    size_t firstBit = firstFrame % PLATFORM_BITS;
    size_t lastBit = (firstFrame + numFrames - 1) % PLATFORM_BITS;

    size_t mask = ~((1ul << (firstBit - 1)) - 1);
    if (firstGroup != lastGroup) {
        assert((bitmap[firstGroup] & mask) == mask);
        bitmap[firstGroup] &= ~mask;

        for (size_t group = firstGroup + 1; group < lastGroup; group++) {
            assert(bitmap[group] == ALL_BITS);
            bitmap[group] = 0;
        }

        mask = ALL_BITS;
    }
    mask &= (1ul << lastBit) - 1;
    assert((bitmap[lastGroup] & mask) == mask);
    bitmap[lastGroup] &= ~mask;*/

    for (size_t i = 0; i < numFrames; i++) {
        invalidateQueue.add(firstFrame+i, freeFrames);
    }
}

/* TODO: optimze, or change to something better than bitmap */
bool Allocator::getFrame(size_t frame) const noexcept {
    return bitmap[frame / FRAMES_PER_GROUP] & (1ul << (frame % FRAMES_PER_GROUP));
}

void Allocator::setFrame(size_t frame) noexcept {
    assert(getFrame(frame) == 0);
    bitmap[frame / FRAMES_PER_GROUP] ^= (1ul << (frame % FRAMES_PER_GROUP));
}

void Allocator::unsetFrame(size_t frame) noexcept {
    assert(getFrame(frame) == 1);
    bitmap[frame / FRAMES_PER_GROUP] ^= (1ul << (frame % FRAMES_PER_GROUP));
}

void *Allocator::map(size_t length,
                     bool findNewFrames,
                     const PhysicalAddress *frames,
                     CacheType cacheType) {
    assert(lock.isLocked());
    assert(length > 0);
    assert(length % PAGE_SIZE == 0);
    assert(findNewFrames == (frames == nullptr));
    if (length > KernelHeapRegion.length) {
        cout << "Kernel MMap attempt larger than kernel heap" << endl;
        throw OutOfKernelHeap();
    }

    size_t numFrames = length / PAGE_SIZE;
    size_t position = startPosition;
    /*size_t stopPostion = startPosition == 0 ? NUM_BITMAP_GROUPS - 1 : startPosition - 1;
    size_t freeBitsAtStart = 0;
    size_t rangeStartGorup = position; */
    size_t numFree = 0;

    while (position != startPosition - 1) { /* TODO: handle startPositon == 0 */
        if (getFrame(position)) {
            numFree = 0;
        } else {
            numFree++;
        }

        if (numFree == numFrames) {
            startPosition = position + 2;
            KernelVirtualAddress startAddress = KernelHeapRegion.start
                    + (position - (numFrames - 1)) * PAGE_SIZE;

            for (size_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
                PhysicalAddress physicalAddress;
                if (findNewFrames) {
                    try {
                        physicalAddress = FrameManagement.get(frameManagement::DEFAULT_ZONE_ID);
                    }  catch (...) {
                        /* allocatePhysicalFrame can fail with OutOfMemory */
                        PagingContext::unmapRange(startAddress, numFrames, true);
                        throw;
                    }
                } else {
                    physicalAddress = frames[frameIndex];
                }

                try {
                    PagingContext::map(startAddress + frameIndex * PAGE_SIZE,
                                       physicalAddress,
                                       Permissions::READ_WRITE,
                                       cacheType);
                } catch (...) {
                    if (findNewFrames) {
                        FrameManagement.put(physicalAddress);
                    }
                    throw;
                }
            }

            do {
                setFrame(position);
                numFrames--;
                position--;
            } while (numFrames > 0);
            return reinterpret_cast<void *>(startAddress.value());
        }

        position++;
        if (position == TOTAL_NUM_HEAP_FRAMES) {
            position = 0;
            numFree = 0;
        }
    }
    throw OutOfKernelHeap();
}

extern "C" void basicInvalidate(KernelVirtualAddress address);

void Allocator::processInvalidateQueue() {
    scoped_lock sl(lock);
    invalidateQueue.localProcessing();
}

void InvalidateQueue::add(size_t frameIndex, bool freeFrame) noexcept {
    KernelVirtualAddress address = KernelHeapRegion.start + frameIndex * PAGE_SIZE;
    basicInvalidate(address);

    if (CurrentSystem.numProcessors == 1) {
        return;
    }

    /* Claer all existing items from this CPU. This is important because we assume Items further
     * from head can only be pending on a subset of Procesors compared to items closer to head */
    localProcessing();

    Item *item = address.asPointer<Item>();
    /* all remaining CPUs that have to process this item */
    item->referenceCount = CurrentSystem.numProcessors - 1;
    item->next = head;
    item->freeFrame = freeFrame;

    CurrentProcessor()->kernelInvalidateProcessedUntil = item;
}

void InvalidateQueue::localProcessing() noexcept {
    KernelVirtualAddress address = head;
    KernelVirtualAddress oldHead = head;

    Processor *currentProcessor = CurrentProcessor();

    while (!address.isNull() && address != currentProcessor->kernelInvalidateProcessedUntil) {
        Item *realItem = address.asPointer<Item>();
        realItem->referenceCount--;

        /* save next pointer, as we are going to invalidate the real one */
        Item item = *realItem;

        while (!realItem->next.isNull()
               && realItem->next != currentProcessor->kernelInvalidateProcessedUntil
               && realItem->next.asPointer<Item>()->referenceCount == 1) {
            /* Next item is going to be fully unmapped. This means we have to update the "next"
             * pointer. Do it now to avoid accessing this item after it has been invalidated */
            realItem->next = nullptr;
        }

        basicInvalidate(address);

        if (item.referenceCount == 0) {
            PagingContext::unmap(address, item.freeFrame);

            assert(address.isPageAligned());
            assert(address.isInRegion(KernelHeapRegion));
            KernelPageAllocator.unsetFrame((address.value() - KernelHeapRegion.start) / PAGE_SIZE);
        }

        address = item.next;
    }
    if (!oldHead.isNull()) {
        currentProcessor->kernelInvalidateProcessedUntil = oldHead;
    }
}

}
