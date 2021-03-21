#include <processes/MemoryArea.hpp>
#include <system/System.hpp>

MemoryArea::MemoryArea(size_t length, Status &) :
    frames(),
    source{Source::ANONYMOUS},
    permissions{Permissions::READ_WRITE_EXECUTE},
    length{length} {

    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(Permissions permissions, size_t length, Status &status) :
    frames(length / PAGE_SIZE, PhysicalAddress::NULL, status),
    source{Source::SHARED},
    permissions{permissions},
    length{length} {

    assert(false);//TODO
    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(size_t frameStack, size_t length, vector<size_t> &deviceAddresses, Status &status):
    frames(length / PAGE_SIZE, PhysicalAddress::NULL, status),
    source{Source::DMA},
    permissions{Permissions::READ_WRITE},
    length{length} {

    if (!status) {
        return;
    }

    size_t numFrames = length / PAGE_SIZE;
    status = deviceAddresses.resize(numFrames);
    if (!status) {
        return;
    }

    for (size_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        Result<PhysicalAddress> addr = Memory::instance()->allocatePhysicalFrame(frameStack);
        if (!addr) {
            status = addr.status();
            return;
        }
        frames[frameIndex] = *addr;
        /* TODO: IO-MMU support */
        deviceAddresses[frameIndex] = addr->value();
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
    if (source == Source::SHARED) {
        for (PhysicalAddress address: frames) {
            /* the constructor may allocate only some frames on failure, so check for NULL */
            if (address != PhysicalAddress::NULL) {
                Memory::instance()->freePhysicalFrame(address);
            }
        }
    }
}

Result<PhysicalAddress> MemoryArea::makePresent(size_t offset) {
    offset = align(offset, PAGE_SIZE, AlignDirection::DOWN);
    assert(offset < length);

    switch (source) {
    case Source::ANONYMOUS:
        return CurrentSystem.memory.allocatePhysicalFrame();
    case Source::PHYSICAL:
        return frames[0] + offset;
    case Source::SHARED: {
        /* TODO: locking / handle second mapping !!! */
        size_t frameIndex = offset / PAGE_SIZE;
        assert(frames[frameIndex] == PhysicalAddress::NULL);

        Result<PhysicalAddress> newFrame = CurrentSystem.memory.allocatePhysicalFrame();
        if (newFrame) {
            frames[frameIndex] = *newFrame;
        }
        return newFrame;
    }
    case Source::DMA:
        size_t frameIndex = offset / PAGE_SIZE;
        assert(frames[frameIndex] != PhysicalAddress::NULL);
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
