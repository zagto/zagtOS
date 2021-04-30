#include <common/common.hpp>
#include <memory/FrameManagement.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <paging/PagingContext.hpp>


namespace kernelPageAllocator {

void Allocator::unmap(void *_address, size_t length, bool freeFrames) {
    assert(lock.isLocked());
    size_t address = KernelVirtualAddress(_address).value();
    Region region(address, length);

    assert(KernelHeapRegion.contains(region));
    assert(region.isPageAligned());

    size_t firstFrame = (address - KernelHeapRegion.start) / PAGE_SIZE;
    size_t numFrames = length / PAGE_SIZE;

    size_t firstGroup = firstFrame / PLATFORM_BITS;
    size_t lastGroup = (firstFrame + numFrames - 1) / PLATFORM_BITS;
    size_t firstBit = firstFrame % PLATFORM_BITS;
    size_t lastBit = (firstFrame + numFrames - 1) % PLATFORM_BITS;

    size_t mask = ~((1ul << (firstBit - 1)) - 1);
    if (firstGroup != lastGroup) {
        assert((bitmap[firstGroup] & mask) == mask);
        bitmap[firstGroup] &= ~mask;

        for (size_t group = firstGroup + 1; group < lastGroup; group++) {
            assert(bitmap[firstGroup] == ALL_BITS);
            bitmap[firstGroup] = 0;
        }

        mask = ALL_BITS;
    }
    mask &= (1ul << lastBit) - 1;
    assert((bitmap[lastGroup] & mask) == mask);
    bitmap[lastGroup] &= ~mask;

    PagingContext::unmapRange(_address, numFrames, freeFrames);
}

/* TODO: optimze, or change to something better than bitmap */
bool Allocator::getFrame(size_t frame) const {
    return bitmap[frame / FRAMES_PER_GROUP] & (1ul << (frame % FRAMES_PER_GROUP));
}

void Allocator::setFrame(size_t frame) {
    assert(getFrame(frame) == 0);
    bitmap[frame / FRAMES_PER_GROUP] ^= (1ul << (frame % FRAMES_PER_GROUP));
}

Result <void *> Allocator::map(size_t length, bool findNewFrames, const PhysicalAddress *frames) {
    assert(lock.isLocked());
    assert(length > 0);
    assert(length % PAGE_SIZE == 0);
    if (length > KernelHeapRegion.length) {
        cout << "Kernel MMap attempt larger than kernel heap" << endl;
        return Status::OutOfKernelHeap();
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
        cout << "startPosition " << startPosition << "position " << position << " numfree " << numFree << endl;

        if (numFree == numFrames) {
                                      /* TODO: support region across startPosition ↓ */
            KernelVirtualAddress startAddress = KernelHeapRegion.start
                    + (position - (numFrames - 1)) * PAGE_SIZE;

            for (size_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
                PhysicalAddress physicalAddress;
                if (findNewFrames) {
                    Result allocResult = FrameManagement.get();
                    if (!allocResult) {
                        /* allocatePhysicalFrame can fail with OutOfMemory */
                        PagingContext::unmapRange(startAddress, numFrames, true);
                        return allocResult.status();
                    }
                    physicalAddress = *allocResult;
                } else {
                    physicalAddress = frames[frameIndex];
                }
                cout << "frameIndex " << frameIndex << " vaddress " << (startAddress.value() + frameIndex * PAGE_SIZE) << endl;
                PagingContext::map(startAddress + frameIndex * PAGE_SIZE,
                                   physicalAddress,
                                   Permissions::READ_WRITE,
                                   CacheType::NORMAL_WRITE_BACK);
            }

            do {
                setFrame(position);
                assert(position > 0);
                numFrames--;
                position--;
            } while (numFrames > 1);
            return reinterpret_cast<void *>(KernelHeapRegion.start + position * PAGE_SIZE);
        }

        position++;
        if (position == TOTAL_NUM_HEAP_FRAMES) {
            position = 0;
            numFree = 0;
        }
    }
    return Status::OutOfKernelHeap();
}

}
