#include <processes/MemoryArea.hpp>
#include <system/System.hpp>

MemoryArea::MemoryArea(size_t length) :
    frames(),
    source{Source::ANONYMOUS},
    permissions{Permissions::READ_WRITE_EXECUTE},
    length{length} {

    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(Permissions permissions, size_t length) :
    frames(length / PAGE_SIZE, PhysicalAddress::NULL),
    source{Source::SHARED},
    permissions{permissions},
    length{length} {

    assert(false);
    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(size_t frameStack, size_t length, vector<size_t> &deviceAddresses):
    frames(length / PAGE_SIZE, PhysicalAddress::NULL),
    source{Source::DMA},
    permissions{Permissions::READ_WRITE},
    length{length} {

    size_t numFrames = length / PAGE_SIZE;
    deviceAddresses.resize(numFrames);
    for (size_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        PhysicalAddress addr = Memory::instance()->allocatePhysicalFrame(frameStack);
        frames[frameIndex] = addr;
        /* TODO: IO-MMU support */
        deviceAddresses[frameIndex] = addr.value();
    }
}


MemoryArea::MemoryArea(Permissions permissions, PhysicalAddress physicalStart, size_t length) :
    frames(1, physicalStart),
    source{Source::PHYSICAL},
    permissions{permissions},
    length{length} {

    assert(physicalStart.isPageAligned());
    assert(length % PAGE_SIZE == 0);
}

MemoryArea::MemoryArea(const hos_v1::MemoryArea &handOver) :
    frames(handOver.numFrames),
    source{handOver.source},
    permissions{handOver.permissions},
    length{handOver.length} {

    assert(source != Source::ANONYMOUS || frames.size() == 0);
    assert(source != Source::PHYSICAL || frames.size() == 1);
    assert(source != Source::SHARED || frames.size() * PAGE_SIZE == length);
    assert(source != Source::DMA || frames.size() * PAGE_SIZE == length);

    for (size_t index = 0; index < frames.size(); index++) {
        frames[index] = handOver.frames[index];
    }
}

PhysicalAddress MemoryArea::makePresent(size_t offset) {
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

        PhysicalAddress newFrame = CurrentSystem.memory.allocatePhysicalFrame();
        frames[frameIndex] = newFrame;
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
