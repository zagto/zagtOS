#include <processes/MemoryArea.hpp>
#include <system/System.hpp>
#include <memory/FrameManagement.hpp>

/* Anonymous */
MemoryArea::MemoryArea(bool shared, Permissions permissions, size_t length, Status &status) :
    frames(length / PAGE_SIZE, nullptr, status),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_ID_NONE, status),
    source{Source::ANONYMOUS},
    permissions{permissions},
    isShared{shared},
    length{length} {

    assert(length % PAGE_SIZE == 0);
    if (!status) {
        return;
    }
}

/* DMA */
MemoryArea::MemoryArea(frameManagement::ZoneID zoneID,
                       size_t length,
                       vector<size_t> &deviceAddresses,
                       Status &status) :
    frames(length / PAGE_SIZE, nullptr, status),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_ID_NONE, status),
    source{Source::DMA},
    permissions{Permissions::READ_WRITE},
    isShared{true},
    length{length} {

    assert(length % PAGE_SIZE == 0);
    if (!status) {
        return;
    }

    for (Frame *&frame : frames) {
        Result result = make_raw<Frame>(zoneID);
        if (!result) {
            status = result.status();
            return;
        }
        frame = *result;
    }

    for (size_t frameIndex = 0; frameIndex < frames.size(); frameIndex++) {
        /* TODO: IO-MMU support */
        deviceAddresses[frameIndex] = frames[frameIndex]->physicalAddress().value();
    }
}

/* Physical */
MemoryArea::MemoryArea(PhysicalAddress physicalStart,
                       size_t length,
                       Status &status) :
    frames(length / PAGE_SIZE, nullptr, status),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_ID_NONE, status),
    source{Source::DMA},
    permissions{Permissions::READ_WRITE},
    physicalStart{physicalStart},
    isShared{true},
    length{length} {

    assert(physicalStart.isPageAligned());
    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(const hos_v1::MemoryArea &handOver, Frame **allFrames, Status &status) :
    frames(handOver.length / PAGE_SIZE, nullptr, status),
    futexIDs(handOver.length / PAGE_SIZE, hos_v1::FUTEX_ID_NONE, status),
    source{handOver.source},
    permissions{handOver.permissions},
    isShared{handOver.isShared},
    length{handOver.length} {

    if (!status) {
        return;
    }

    for (size_t frameIndex = 0; frameIndex < frames.size(); frameIndex++) {
        frames[frameIndex] = allFrames[handOver.frameIDs[frameIndex]];
        futexIDs[frameIndex] = handOver.futexIDs[frameIndex];
    }
}

MemoryArea::~MemoryArea() {
    for (Frame *&frame : frames) {
        if (frame != nullptr) {
            frame->decreaseMemoryAreaReference();
            frame = nullptr;
        }
    }
}

Status MemoryArea::ensureFramePresent(size_t frameIndex, bool requireNoCopyOnWrite) {
    assert(lock.isLocked());

    if (frames[frameIndex] == nullptr) {
        Result<Frame *> result;
        switch (source) {
        case Source::ANONYMOUS:
            result = make_raw<Frame>(frameManagement::DEFAULT_ZONE_ID);
            break;
        case Source::PHYSICAL:
            result = make_raw<Frame>(physicalStart + (frameIndex * PAGE_SIZE), true);
            break;
        case Source::DMA:
            cout << "DMA type MemoryAreas cannot be lazy initialized." << endl;
            Panic();
        }

        if (!result) {
            return result.status();
        }
        frames[frameIndex] = *result;
    }

    if (requireNoCopyOnWrite && frames[frameIndex]->isCopyOnWrite()) {
        /* There should never be copy-on-write in shared memory */
        assert(!isShared);
        Result<Frame *> result = frames[frameIndex]->ensureNoCopyOnWrite();
        if (!result) {
            return result.status();
        }

        frames[frameIndex] = *result;
    }

    return Status::OK();
}

Status MemoryArea::pageIn(ProcessAddressSpace &addressSpace,
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
    Status status = ensureFramePresent(frameIndex, requireNoCopyOnWrite);
    if (!status) {
        return status;
    }

    status = frames[frameIndex]->pageIn(addressSpace, address, mappingPermissions);
    if (!status) {
        return status;
    }

    return Status::OK();
}

PageOutContext MemoryArea::pageOut(ProcessAddressSpace &addressSpace,
                                   UserVirtualAddress address,
                                   size_t offset) {
    assert(address.isPageAligned());
    assert(offset % PAGE_SIZE == 0);
    assert(offset < length);

    size_t frameIndex = offset / PAGE_SIZE;
    assert(frames[frameIndex] != nullptr);

    scoped_lock sl(lock);

    return frames[frameIndex]->pageOut(addressSpace, address);
}


bool MemoryArea::allowesPermissions(Permissions toTest) const {
    assert(permissions != Permissions::INVALID);

    if (toTest == Permissions::INVALID) {
        return true;
    }
    switch (permissions) {
    case Permissions::READ_WRITE_EXECUTE:
        return true;
    case Permissions::READ_WRITE:
        return toTest == Permissions::READ || toTest == Permissions::READ_WRITE;
    case Permissions::READ_EXECUTE:
        return toTest == Permissions::READ || toTest == Permissions::READ_EXECUTE;
    case Permissions::READ:
        return toTest == Permissions::READ;
    default:
        cout << "not implemented permission" << endl;
        Panic();
    }
}

Result<uint64_t> MemoryArea::getFutexID(size_t offset) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(offset < length);

    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    Status status = ensureFramePresent(offset / PAGE_SIZE, true);
    if (!status) {
        return status;
    }

    FutexFrameID futexFrameID = CurrentSystem.getNewFutexFrameID();
    futexIDs[frameIndex] = futexFrameID;
    return (futexFrameID << PAGE_SHIFT) | offset % PAGE_SIZE;
}

Status MemoryArea::_copyFrom(uint8_t *destination,
                             size_t offset,
                             size_t accessLength,
                             scoped_lock<mutex> &scopedLock) {
    assert(offset < length);
    assert(offset + accessLength < length);

    size_t frameIndex = offset / PAGE_SIZE;
    size_t destPosition = 0;

    size_t offsetInFrame = offset % PAGE_SIZE;

    while (destPosition < accessLength) {
        size_t partLength = max(PAGE_SIZE - offsetInFrame, accessLength - destPosition);

        Status status = ensureFramePresent(offset / PAGE_SIZE, false);
        if (!status) {
            return status;
        }

        status = frames[frameIndex]->copyFrom(destination + destPosition,
                                              offsetInFrame,
                                              partLength);
        if (!status) {
            return status;
        }

        destPosition += partLength;
        frameIndex++;
        offsetInFrame = 0;

        scopedLock.checkWaiters();
    }
    return Status::OK();
}

Status MemoryArea::copyFrom(uint8_t *destination, size_t offset, size_t accessLength) {
    scoped_lock sl(lock);
    return _copyFrom(destination, offset, accessLength, sl);
}


Status MemoryArea::copyTo(size_t offset, const uint8_t *source, size_t accessLength) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(offset < length);
    assert(offset + accessLength < length);

    scoped_lock sl(lock);

    size_t frameIndex = offset / PAGE_SIZE;
    size_t sourcePosition = 0;

    size_t offsetInFrame = offset % PAGE_SIZE;

    while (sourcePosition < accessLength) {
        size_t partLength = max(PAGE_SIZE - offsetInFrame, accessLength - sourcePosition);

        Status status = ensureFramePresent(offset / PAGE_SIZE, true);
        if (!status) {
            return status;
        }

        status = frames[frameIndex]->copyTo(offsetInFrame,
                                            source + sourcePosition,
                                            partLength);

        if (!status) {
            return status;
        }

        sourcePosition += partLength;
        frameIndex++;
        offsetInFrame = 0;

        sl.checkWaiters();
    }
    return Status::OK();
}

Status MemoryArea::copyFromOther(size_t destinationOffset,
                                 MemoryArea &sourceArea,
                                 size_t sourceOffset,
                                 size_t accessLength) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(sourceOffset < sourceArea.length);
    assert(destinationOffset < length);
    assert(sourceOffset + accessLength < sourceArea.length);
    assert(destinationOffset + accessLength < length);

    scoped_lock sl(lock);

    size_t frameIndex = destinationOffset / PAGE_SIZE;
    size_t position = 0;

    size_t offsetInFrame = destinationOffset % PAGE_SIZE;

    while (position < accessLength) {
        size_t partLength = max(PAGE_SIZE - offsetInFrame, accessLength - position);

        Status status = ensureFramePresent(destinationOffset / PAGE_SIZE, true);
        if (!status) {
            return status;
        }

        status = frames[frameIndex]->copyFromMemoryArea(offsetInFrame,
                                                        sourceArea,
                                                        sourceOffset + position,
                                                        partLength,
                                                        sl);
        if (!status) {
            return status;
        }

        position += partLength;
        frameIndex++;
        offsetInFrame = 0;
    }
    return Status::OK();
}

Result<uint32_t> MemoryArea::atomicCopyFrom32(size_t offset) {
    assert(offset < length);
    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    Status status = ensureFramePresent(frameIndex, false);
    if (!status) {
        return status;
    }

    return frames[frameIndex]->atomicCopyFrom32(offset % PAGE_SIZE);
}

Result<bool> MemoryArea::atomicCompareExchange32(size_t offset,
                                                 uint32_t expectedValue,
                                                 uint32_t newValue) {
    assert(allowesPermissions(Permissions::READ_WRITE));
    assert(offset < length);
    size_t frameIndex = offset / PAGE_SIZE;

    scoped_lock sl(lock);

    Status status = ensureFramePresent(frameIndex, true);
    if (!status) {
        return status;
    }

    return frames[frameIndex]->atomicCompareExchange32(offset % PAGE_SIZE, expectedValue, newValue);
}
