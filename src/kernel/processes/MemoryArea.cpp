#include <processes/MemoryArea.hpp>
#include <system/System.hpp>
#include <memory/FrameManagement.hpp>

MemoryArea::MemoryArea(bool shared, Permissions permissions, size_t length, Status &status) :
    frames(length / PAGE_SIZE, nullptr, status),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_ID_NONE, status),
    isShared{shared},
    source{Source::ANONYMOUS},
    permissions{permissions},
    length{length} {

    assert(length % PAGE_SIZE == 0);
    if (!status) {
        return;
    }
}

MemoryArea::MemoryArea(frameManagement::ZoneID zoneID,
                       size_t length,
                       vector<size_t> &deviceAddresses,
                       Status &status) :
    frames(length / PAGE_SIZE, nullptr, status),
    futexIDs(length / PAGE_SIZE, hos_v1::FUTEX_ID_NONE, status),
    isShared{true},
    source{Source::DMA},
    permissions{Permissions::READ_WRITE},
    length{length} {

    assert(length % PAGE_SIZE == 0);
    if (!status) {
        return;
    }

    for (Frame *&frame : frames) {
        Result<PhysicalAddress> addr = FrameManagement.get(zoneID);
        if (!addr) {
            status = addr.status();
            return;
        }
        Result result = make_raw<Frame>(status);
        if (!result) {
            FrameManagement.put(*addr);
            status = result.status();
            return;
        }
    }

    for (size_t frameIndex = 0; frameIndex < frames.size(); frameIndex++) {
        /* TODO: IO-MMU support */
        deviceAddresses[frameIndex] = frames[frameIndex]->address.value();
    }
}


MemoryArea::MemoryArea(Permissions permissions,
                       PhysicalAddress physicalStart,
                       size_t length,
                       Status &status) :
    frames(1, physicalStart, status),
    source{Source::PHYSICAL},
    permissions{permissions},
    length{length} {

    assert(physicalStart.isPageAligned());
    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(const hos_v1::MemoryArea &handOver, Status &status) :
    frames(handOver.numFrames, status),
    source{handOver.source},
    permissions{handOver.permissions},
    length{handOver.length} {

    if (!status) {
        return;
    }

    assert(source != Source::ANONYMOUS || frames.size() == 0);
    assert(source != Source::PHYSICAL || frames.size() == 1);
    assert(source != Source::SHARED || frames.size() * PAGE_SIZE == length);
    assert(source != Source::DMA || frames.size() * PAGE_SIZE == length);

    for (size_t index = 0; index < frames.size(); index++) {
        frames[index] = handOver.frames[index];
    }
}

MemoryArea::~MemoryArea() {
    for (Frame *&frame : frames) {

        if (source != Source::PHYSICAL) {
            FrameManagement.put(frame->address);
        }
        for (PhysicalAddress address: frames) {
            /* the constructor may allocate only some frames on failure, so check for NULL */
            if (address.value() != PhysicalAddress::NULL) {
            }
        }
    }
}

Result<PhysicalAddress> MemoryArea::makePresent(size_t offset) {
    offset = align(offset, PAGE_SIZE, AlignDirection::DOWN);
    assert(offset < length);

    switch (source) {
    case Source::ANONYMOUS:
        return FrameManagement.get();
    case Source::PHYSICAL:
        return frames[0] + offset;
    case Source::SHARED: {
        /* TODO: locking / handle second mapping !!! */
        size_t frameIndex = offset / PAGE_SIZE;
        assert(frames[frameIndex] == PhysicalAddress::NULL);

        Result<PhysicalAddress> newFrame = FrameManagement.get();
        if (newFrame) {
            frames[frameIndex] = *newFrame;
        }
        return newFrame;
    }
    case Source::DMA:
        size_t frameIndex = offset / PAGE_SIZE;
        assert(frames[frameIndex].value() != PhysicalAddress::NULL);
        return frames[frameIndex];
    }

    cout << "unimplemented memeory source" << endl;
    Panic();
}

CacheType MemoryArea::cacheType() const {
    if (source == Source::PHYSICAL) {
        return CacheType::NONE;
    } else {
        return CacheType::NORMAL_WRITE_BACK;
    }
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

bool MemoryArea::isAnonymous() const {
    return source == Source::ANONYMOUS;
}


Result<uint32_t> atomicCompareExchange32(size_t offset,
                                         uint32_t expectedValue,
                                         uint32_t newValue) {
    scoped_lock sl(lock);
}
