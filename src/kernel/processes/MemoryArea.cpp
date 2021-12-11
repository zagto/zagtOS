#include <processes/MemoryArea.hpp>
#include <system/System.hpp>
#include <memory/FrameManagement.hpp>
#include <memory>

/* Anonymous */
MemoryArea::MemoryArea(bool shared, Permissions permissions, size_t length) :
    frames(length / PAGE_SIZE, nullptr),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_FRAME_ID_NONE),
    source{Source::ANONYMOUS},
    permissions{permissions},
    isShared{shared},
    length{length} {

    assert(length % PAGE_SIZE == 0);
}

/* DMA */
MemoryArea::MemoryArea(frameManagement::ZoneID zoneID,
                       size_t length,
                       vector<size_t> &deviceAddresses) :
    frames(length / PAGE_SIZE, nullptr),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_FRAME_ID_NONE),
    source{Source::DMA},
    permissions{Permissions::READ_WRITE},
    isShared{true},
    length{length} {

    assert(length % PAGE_SIZE == 0);

    deviceAddresses = vector<size_t>(frames.size(), PhysicalAddress::Null);

    try {
        for (Frame *&frame : frames) {
            frame = new Frame(zoneID);
        }

        for (size_t frameIndex = 0; frameIndex < frames.size(); frameIndex++) {
            /* TODO: IO-MMU support */
            deviceAddresses[frameIndex] = frames[frameIndex]->physicalAddress().value();
        }
    } catch(...) {
        discardFrames();
    }
}

/* Physical */
MemoryArea::MemoryArea(PhysicalAddress physicalStart,
                       size_t length) :
    frames(length / PAGE_SIZE, nullptr),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_FRAME_ID_NONE),
    source{Source::PHYSICAL},
    permissions{Permissions::READ_WRITE},
    physicalStart{physicalStart},
    isShared{true},
    length{length} {

    assert(physicalStart.isPageAligned());
    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(const hos_v1::MemoryArea &handOver,
                       vector<Frame *> &allFrames) :
    frames(handOver.length / PAGE_SIZE, nullptr),
    futexIDs(handOver.length / PAGE_SIZE, hos_v1::FUTEX_FRAME_ID_NONE),
    source{handOver.source},
    permissions{handOver.permissions},
    isShared{handOver.isShared},
    length{handOver.length} {

    for (size_t frameIndex = 0; frameIndex < frames.size(); frameIndex++) {
        frames[frameIndex] = allFrames[handOver.frameIDs[frameIndex]];
        futexIDs[frameIndex] = handOver.futexIDs[frameIndex];
    }
}

MemoryArea::MemoryArea(MemoryArea &other, size_t offset, size_t length):
    frames(length / PAGE_SIZE, nullptr),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_FRAME_ID_NONE),
    source{other.source},
    permissions{other.permissions},
    isShared{other.isShared},
    length{length} {

    assert(offset <= other.length);
    assert(offset + length <= other.length);
    assert(offset % PAGE_SIZE == 0);
    assert(length % PAGE_SIZE == 0);

    scoped_lock sl(other.lock);

    for (size_t frameIndex = 0; frameIndex < length / PAGE_SIZE; frameIndex++) {
        Frame *frame = other.frames[offset / PAGE_SIZE + frameIndex];
        if (frame != nullptr) {
            frame->copyOnWriteDuplicate();
        }
        frames[frameIndex] = frame;
    }
}

MemoryArea::~MemoryArea() {
    discardFrames();
}

void MemoryArea::discardFrames() noexcept {
    for (Frame *&frame : frames) {
        if (frame != nullptr) {
            frame->decreaseMemoryAreaReference();
            frame = nullptr;
        }
    }
}

void MemoryArea::ensureFramePresent(size_t frameIndex, bool requireNoCopyOnWrite) {
    assert(lock.isLocked());

    if (frames[frameIndex] == nullptr) {
        switch (source) {
        case Source::ANONYMOUS:
            frames[frameIndex] = new Frame(frameManagement::DEFAULT_ZONE_ID);
            break;
        case Source::PHYSICAL:
            frames[frameIndex] = new Frame(physicalStart + (frameIndex * PAGE_SIZE), true);
            break;
        case Source::DMA:
            cout << "DMA type MemoryAreas cannot be lazy initialized." << endl;
            Panic();
        }
    }

    if (requireNoCopyOnWrite && frames[frameIndex]->isCopyOnWrite()) {
        /* There should never be copy-on-write in shared memory */
        assert(!isShared);
        frames[frameIndex] = frames[frameIndex]->ensureNoCopyOnWrite();
    }
}

void MemoryArea::pageIn(ProcessAddressSpace &addressSpace,
                          UserVirtualAddress address,
                          Permissions mappingPermissions,
                          size_t offset) {
    assert(address.isPageAligned());
    assert(offset % PAGE_SIZE == 0);
    assert(offset < length);

    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    /* if a page in a writeable MemoryArea, copy it is copied as soon as it is mapped. This makes
     * things a lot easier to implement and also saves us from an extra TLB shootdown if the frame
     * is written later */
    bool requireNoCopyOnWrite = allowesPermissions(Permissions::READ_WRITE);

    ensureFramePresent(frameIndex, requireNoCopyOnWrite);
    frames[frameIndex]->pageIn(addressSpace, address, mappingPermissions);
}

PageOutContext MemoryArea::pageOut(ProcessAddressSpace &addressSpace,
                                   UserVirtualAddress address,
                                   size_t offset) noexcept {
    assert(address.isPageAligned());
    assert(offset % PAGE_SIZE == 0);
    assert(offset < length);

    size_t frameIndex = offset / PAGE_SIZE;
    assert(frames[frameIndex] != nullptr);

    scoped_lock sl(lock);

    return frames[frameIndex]->pageOut(addressSpace, address);
}


bool MemoryArea::allowesPermissions(Permissions toTest) const noexcept {
    assert(permissions != Permissions::INVALID);

    if (toTest == Permissions::INVALID) {
        return true;
    }

    return permissions >= toTest;
}

uint64_t MemoryArea::getFutexID(size_t offset) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(offset < length);

    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    ensureFramePresent(offset / PAGE_SIZE, true);

    FutexFrameID &futexFrameID = futexIDs[frameIndex];
    if (futexFrameID == hos_v1::FUTEX_FRAME_ID_NONE) {
        futexFrameID = CurrentSystem.getNewFutexFrameID();
    }
    return (futexFrameID << PAGE_SHIFT) | offset % PAGE_SIZE;
}

void MemoryArea::_copyFrom(uint8_t *destination,
                             size_t offset,
                             size_t accessLength,
                             optional<scoped_lock<mutex> *> relaxLock) {
    assert(offset < length);
    assert(offset + accessLength <= length);

    size_t frameIndex = offset / PAGE_SIZE;
    size_t destPosition = 0;

    size_t offsetInFrame = offset % PAGE_SIZE;

    while (destPosition < accessLength) {
        size_t partLength = min(PAGE_SIZE - offsetInFrame, accessLength - destPosition);

        ensureFramePresent(offset / PAGE_SIZE, false);

        frames[frameIndex]->copyFrom(destination + destPosition,
                                     offsetInFrame,
                                     partLength);
        destPosition += partLength;
        frameIndex++;
        offsetInFrame = 0;

        if (relaxLock) {
            (*relaxLock)->checkWaiters();
        }
    }
}

void MemoryArea::copyFrom(uint8_t *destination, size_t offset, size_t accessLength) {
    scoped_lock sl(lock);
    _copyFrom(destination, offset, accessLength, &sl);
}


void MemoryArea::copyTo(size_t offset, const uint8_t *source, size_t accessLength) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(offset < length);
    assert(offset + accessLength <= length);

    scoped_lock sl(lock);

    size_t frameIndex = offset / PAGE_SIZE;
    size_t sourcePosition = 0;

    size_t offsetInFrame = offset % PAGE_SIZE;

    while (sourcePosition < accessLength) {
        size_t partLength = min(PAGE_SIZE - offsetInFrame, accessLength - sourcePosition);

        ensureFramePresent(offset / PAGE_SIZE, true);

        frames[frameIndex]->copyTo(offsetInFrame,
                            source + sourcePosition,
                            partLength);

        sourcePosition += partLength;
        frameIndex++;
        offsetInFrame = 0;

        sl.checkWaiters();
    }
}

void MemoryArea::copyFromOther(size_t destinationOffset,
                               MemoryArea &sourceArea,
                               size_t sourceOffset,
                               size_t accessLength) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(sourceOffset < sourceArea.length);
    assert(destinationOffset < length);
    assert(sourceOffset + accessLength <= sourceArea.length);
    assert(destinationOffset + accessLength <= length);

    scoped_lock sl(lock, sourceArea.lock);

    size_t frameIndex = destinationOffset / PAGE_SIZE;
    size_t position = 0;

    size_t offsetInFrame = destinationOffset % PAGE_SIZE;

    while (position < accessLength) {
        size_t partLength = min(PAGE_SIZE - offsetInFrame, accessLength - position);

        ensureFramePresent(frameIndex, true);

        frames[frameIndex]->copyFromMemoryArea(offsetInFrame,
                                               sourceArea,
                                               sourceOffset + position,
                                               partLength);
        position += partLength;
        frameIndex++;
        offsetInFrame = 0;

        sl.checkWaiters();
    }
}

uint32_t MemoryArea::atomicCopyFrom32(size_t offset) {
    assert(offset < length);
    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    ensureFramePresent(frameIndex, false);
    return frames[frameIndex]->atomicCopyFrom32(offset % PAGE_SIZE);
}

bool MemoryArea::atomicCompareExchange32(size_t offset,
                                                 uint32_t expectedValue,
                                                 uint32_t newValue) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(offset < length);
    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    ensureFramePresent(frameIndex, true);
    return frames[frameIndex]->atomicCompareExchange32(offset % PAGE_SIZE, expectedValue, newValue);
}
