#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <memory/dlmalloc-glue.hpp>
#include <paging/PagingContext.hpp>


static constexpr size_t TOTAL_NUM_HEAP_FRAMES = KernelHeapRegion.length / PAGE_SIZE;
static constexpr size_t FRAMES_PER_GROUP = PLATFORM_BITS;
static constexpr size_t NUM_BITMAP_GROUPS = TOTAL_NUM_HEAP_FRAMES / FRAMES_PER_GROUP;
static constexpr size_t ALL_BITS = static_cast<size_t>(-1);
#define MFAIL reinterpret_cast<void *>(ALL_BITS)

static size_t bitmap[NUM_BITMAP_GROUPS];
static size_t startPosition = 0;

static_assert(KernelHeapRegion.length % PAGE_SIZE == 0);
static_assert(TOTAL_NUM_HEAP_FRAMES % FRAMES_PER_GROUP == 0);

Status DLMallocStatus = Status::OK();


extern "C" void BasicDLMallocPanic(const char *location) {
    cout << "DLMalloc problem occured at: " << location << "\n";
    Panic();
}

extern "C" int KernelMUnmap(void *_address, size_t length) {
    size_t address = reinterpret_cast<size_t>(_address);
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

    return 0;
}

/* TODO: optimze, or change to something better than bitmap */
static bool getFrame(size_t frame) {
    return bitmap[frame / FRAMES_PER_GROUP] & (1ul << (frame % FRAMES_PER_GROUP));
}

static void setFrame(size_t frame) {
    assert(getFrame(frame) == 0);
    bitmap[frame / FRAMES_PER_GROUP] ^= (1ul << (frame % FRAMES_PER_GROUP));
}

extern "C" void *KernelMMap(size_t length) {
    assert(length % PAGE_SIZE == 0);
    if (length > KernelHeapRegion.length) {
        cout << "Kernel MMap attempt larger than kernel heap" << endl;
        DLMallocStatus = Status::OutOfKernelHeap();
        return MFAIL;
    }

    size_t numFrames = length / PAGE_SIZE;
    size_t position = startPosition;
    /*size_t stopPostion = startPosition == 0 ? NUM_BITMAP_GROUPS - 1 : startPosition - 1;
    size_t freeBitsAtStart = 0;
    size_t rangeStartGorup = position; */
    size_t numFree = 0;

    while (position != startPosition - 1) {
        if (getFrame(position)) {
            numFree = 0;
        } else {
            numFree++;
        }

        if (numFree == numFrames) {
            KernelVirtualAddress startAddress = KernelHeapRegion.start + (position - numFrames) * PAGE_SIZE;

            for (size_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
                Result<PhysicalAddress> physicalAddress = Memory::instance()->allocatePhysicalFrame();
                if (!physicalAddress) {
                    /* allocatePhysicalFrame can fail with OutOfMemory */
                    PagingContext::unmapRange(startAddress, numFrames, true);
                    DLMallocStatus = physicalAddress.status();
                    return MFAIL;
                }
                PagingContext::map(startAddress + frameIndex * PAGE_SIZE,
                                   *physicalAddress,
                                   Permissions::READ_WRITE,
                                   CacheType::NORMAL_WRITE_BACK);
            }

            while (numFrames > 0) {
                setFrame(position);
                numFrames--;
                if (position > 0) {
                    position--;
                } else {
                    position = TOTAL_NUM_HEAP_FRAMES - 1;
                }
            }
            return reinterpret_cast<void *>(KernelHeapRegion.start + position * PAGE_SIZE);
        }

        if (position == TOTAL_NUM_HEAP_FRAMES) {
            position++;
        } else {
            position = 0;
            numFree = 0;
        }
    }
    /* TODO: check region across startPosition */
    DLMallocStatus = Status::OutOfKernelHeap();
    return MFAIL;
}
