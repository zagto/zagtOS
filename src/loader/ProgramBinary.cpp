#include <ProgramBinary.hpp>
#include <log/Logger.hpp>
#include <Paging.hpp>
#include <memory/PhysicalMemory.hpp>
#include <common/utils.hpp>
#include <memory/ArchRegions.hpp>

/* Rather inefficient but fully in-place parsing for ZBON ProgramBinaries. We can't use the regular
 * user-spave implementation because it needs dynamic allocation due to the nature of the binary
 * format - they can have any number of sections. */

enum Type : uint8_t {
    BINARY = 1, NOTHING, OBJECT, STRING, BOOLEAN,
    INT8, UINT8, INT16, UINT16, INT32, UINT32, INT64, UINT64,
    FLOAT, DOUBLE, HANDLE, NUM_TYPES
};

static const size_t FLAG_EXECUTABLE{1};
static const size_t FLAG_WRITEABLE{2};
static const size_t FLAG_READABLE{4};

/* Header size for objects: type byte, number of elements, number of handles, size in bytes */
static const size_t OBJECT_HEADER_SIZE = 1 + 3*8;
/* Header size for flat binary data: type byte, size in bytes */
static const size_t BINARY_HEADER_SIZE = 1 + 8;

size_t ProgramBinary::readSize(size_t offset) const {
    uint64_t result = 0;
    for (int byte = 7; byte >= 0; byte--) {
        result = (result << 8) | data[offset + byte];
    }
    assert(sizeof(size_t) == 8 || result >= 0xffffffffu);
    return result;
}

size_t ProgramBinary::TLSOffset() const {
    /* after UINT64 entry address with it's type byte */
    return OBJECT_HEADER_SIZE + 1 + 8;
}

size_t ProgramBinary::sectionsArrayOffset() const {
    if (data[TLSOffset()] == OBJECT) {
        return TLSOffset() + OBJECT_HEADER_SIZE + readSize(TLSOffset() + 1 + 2*8);
    } else {
        return TLSOffset() + 1;
    }
}

size_t ProgramBinary::numSections() const {
    return data[sectionsArrayOffset() + 1];
}

size_t ProgramBinary::sectionOffset(size_t index) const {
    assert(index < numSections());

    if (index == 0) {
        return sectionsArrayOffset() + OBJECT_HEADER_SIZE;
    } else {
        return sectionOffset(index - 1)
                + OBJECT_HEADER_SIZE
                + readSize(sectionOffset(index - 1) + 1 + 2*8);
    }
}

size_t ProgramBinary::sectionAddress(size_t sectionOffset) const {
    return readSize(sectionOffset + OBJECT_HEADER_SIZE + 1);
}
size_t ProgramBinary::sectionSizeInMemory(size_t sectionOffset) const {
    return readSize(sectionOffset + OBJECT_HEADER_SIZE + 1 + 9);
}
size_t ProgramBinary::sectionFlags(size_t sectionOffset) const {
    return readSize(sectionOffset + OBJECT_HEADER_SIZE + 1 + 2*9);
}
size_t ProgramBinary::sectionDataSize(size_t sectionOffset) const {
    return readSize(sectionOffset + OBJECT_HEADER_SIZE + 1 + 3*9);
}
size_t ProgramBinary::sectionDataOffset(size_t sectionOffset) const {
    return sectionOffset + OBJECT_HEADER_SIZE + BINARY_HEADER_SIZE + 3*9;
}
Region ProgramBinary::sectionRegion(size_t sectionOffset) const {
    Region result{sectionAddress(sectionOffset), sectionSizeInMemory(sectionOffset)};
    alignedGrow(result.start, result.length, PAGE_SIZE);
    return result;
}

size_t ProgramBinary::loadedUserFrames() const {
    size_t sum = 0;
    for (size_t i = 0; i < numSections(); i++) {
        size_t sectionSize = sectionSizeInMemory(sectionOffset(i));
        assert(sectionSize % PAGE_SIZE == 0);
        sum += sectionSize / PAGE_SIZE;
    }

    /* run message has one frame */
    sum++;

    if (hasTLS()) {
        size_t sectionSize = sectionSizeInMemory(TLSOffset());
        assert(sectionSize % PAGE_SIZE == 0);
        sum += sectionSize / PAGE_SIZE;
    }

    return sum;
}


void ProgramBinary::sanityCheckSection(size_t offset) const {
    /* the section is an object */
    assert(data[offset] == OBJECT);
    /* it has 3 elements */
    assert(readSize(offset + 1) == 4);
    /* it has no handles */
    assert(readSize(offset + 9) == 0);

    /* address type */
    assert(data[offset + OBJECT_HEADER_SIZE] == UINT64);
    /* sizeInMemory type */
    assert(data[offset + OBJECT_HEADER_SIZE + 9] == UINT64);
    /* flags type */
    assert(data[offset + OBJECT_HEADER_SIZE + 2*9] == UINT64);
    /* data type */
    assert(data[offset + OBJECT_HEADER_SIZE + 3*9] == BINARY);
}

void ProgramBinary::sanityChecks() const {
    /* the whole thing is an object */
    assert(data[0] == OBJECT);
    /* it has 3 elements */
    assert(readSize(1) == 3);
    /* it has no handles */
    assert(readSize(9) == 0);

    /* entry address type */
    assert(data[OBJECT_HEADER_SIZE] == UINT64);

    /* TLS type */
    assert(data[TLSOffset()] == NOTHING || data[TLSOffset()] == OBJECT);
    if (data[TLSOffset()] == OBJECT) {
        sanityCheckSection(TLSOffset());
    }

    cout << "sectionsArrayOffset " << sectionsArrayOffset() << endl;
    assert(data[sectionsArrayOffset()] == OBJECT);
    for (size_t index = 0; index < numSections(); index++) {
        cout << "checking section " << index << " at offset " << sectionOffset(index) << endl;
        sanityCheckSection(sectionOffset(index));
    }
}

ProgramBinary::ProgramBinary(void *pointer) {
    data = reinterpret_cast<uint8_t *>(pointer);
    sanityChecks();
}

bool ProgramBinary::hasTLS() const {
    return data[TLSOffset()] == OBJECT;
}

size_t ProgramBinary::entryAddress() const {
    return readSize(OBJECT_HEADER_SIZE + 1);
}

UserVirtualAddress ProgramBinary::TLSBase() const {
    if (hasTLS()) {
        return _TLSBase;
    } else {
        return {};
    }
}

UserVirtualAddress ProgramBinary::masterTLSBase() const {
    if (hasTLS()) {
        return sectionAddress(TLSOffset());
    } else {
        return {};
    }
}

Region ProgramBinary::TLSRegion() const {
    if (hasTLS()) {
        return _TLSRegion;
    } else {
        return {0, 0};
    }
}

UserVirtualAddress ProgramBinary::runMessageAddress() const {
    for (size_t insertIndex = 0; insertIndex < numSections(); insertIndex++) {
        Region usedRegion = sectionRegion(sectionOffset(insertIndex));
        Region testRegion = Region{usedRegion.end(), PAGE_SIZE};
        bool conflicts = false;

        for (size_t compareIndex = 0; compareIndex < numSections(); compareIndex++) {
            if (sectionRegion(sectionOffset(insertIndex)).overlaps(testRegion)) {
                conflicts = true;
            }
        }
        if (!UserSpaceRegion.contains(testRegion)) {
            conflicts = true;
        }

        if (!conflicts) {
            return testRegion.start;
        }
    }
    cout << "could not find region to place run message" << endl;
    Panic();
}

hos_v1::Permissions ProgramBinary::sectionPermissions(size_t sectionOffset) const {
    size_t flags = sectionFlags(sectionOffset);
    switch (flags) {
    case FLAG_READABLE | FLAG_WRITEABLE | FLAG_EXECUTABLE:
        return Permissions::READ_WRITE_EXECUTE;
    case FLAG_READABLE | FLAG_EXECUTABLE:
        return Permissions::READ_EXECUTE;
    case FLAG_READABLE | FLAG_WRITEABLE:
        return Permissions::READ_WRITE;
    case FLAG_READABLE:
        return Permissions::READ;
    default:
        return Permissions::INVALID;
    }
}

void ProgramBinary::load(PagingContext pagingContext,
                         hos_v1::Frame *frames,
                         size_t &frameIndex) {
    for (size_t sectionIndex = 0; sectionIndex < numSections(); sectionIndex++) {
        size_t offset = sectionOffset(sectionIndex);

        cout << "section " << sectionIndex  << " offset " << offset << " dataOffset" << sectionDataOffset(offset) << endl;
        Region region = sectionRegion(offset);
        size_t flags = sectionFlags(offset);
        uint8_t *source = data + sectionDataOffset(offset);
        size_t sourceLen = sectionDataSize(offset);

        assert(region.isPageAligned());
        assert(sourceLen % PAGE_SIZE == 0);
        assert(!((flags & FLAG_WRITEABLE) && (flags & FLAG_EXECUTABLE)));

        cout << "loading section to " << region.start << " length " << region.length << endl;

        for (size_t pageIndex = 0; pageIndex < region.length / PAGE_SIZE; pageIndex++) {
            PhysicalAddress physicalAddress = AllocatePhysicalFrame();
            /* if there is anything to be written to this page */
            if (sourceLen > pageIndex * PAGE_SIZE) {
                memcpy(reinterpret_cast<void *>(physicalAddress.value()),
                       source + pageIndex * PAGE_SIZE,
                       PAGE_SIZE);
            }

            if (pagingContext == PagingContext::GLOBAL) {
                MapAddress(pagingContext,
                           region.start + pageIndex * PAGE_SIZE,
                           physicalAddress,
                           flags & FLAG_WRITEABLE,
                           flags & FLAG_EXECUTABLE,
                           false,
                           CacheType::CACHE_NORMAL_WRITE_BACK);
            } else {
                frames[frameIndex] = hos_v1::Frame{
                    .address = physicalAddress.value(),
                    .copyOnWriteCount = 1,
                };
                frameIndex++;
            }
        }
    }

    /* if this is a user-space program, load run message */
    if (pagingContext == PagingContext::PROCESS) {
        /* put run message directly below stack */
        PhysicalAddress physicalAddress = AllocatePhysicalFrame();
        struct UserMessageInfo {
            uint8_t type[16];
            size_t address;
            size_t length;
            size_t numHandles;
            /* externally for user = kernel */
            bool allocatedExternally;
        };
        UserMessageInfo *msgInfo = reinterpret_cast<UserMessageInfo *>(physicalAddress.value());
        *msgInfo = {
            .type = {0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
                     0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55}, /* MSG_BE_INIT */
            .address = runMessageAddress().value(),
            .length = 0,
            .numHandles = 0,
            .allocatedExternally = true
        };

        frames[frameIndex] = hos_v1::Frame{
            .address = physicalAddress.value(),
            .copyOnWriteCount = 1,
        };
        frameIndex++;
    }

    if (hasTLS()) {
        size_t offset = TLSOffset();
        bool foundTLSRegion{false};

        _TLSRegion = sectionRegion(offset);
        for (size_t startAfterIndex = 0; startAfterIndex < numSections(); startAfterIndex++) {
            _TLSRegion.start = sectionRegion(sectionOffset(startAfterIndex)).end();
            bool conflicts{false};

            for (size_t checkConflictIndex = 0;
                 checkConflictIndex < numSections();
                 checkConflictIndex++) {
                Region otherRegion = sectionRegion(sectionOffset(checkConflictIndex));
                if (_TLSRegion.overlaps(otherRegion)) {
                    conflicts = true;
                }
            }
            if (_TLSRegion.overlaps(Region{runMessageAddress().value(), PAGE_SIZE})) {
                conflicts = true;
            }
            if (!conflicts) {
                foundTLSRegion = true;
                break;
            }
        }

        if (!foundTLSRegion) {
            cout << "Unable to find suitable region for TLS" << endl;
            Halt();
        }

        uint8_t *source = data + sectionDataOffset(offset);
        size_t sourceLen = sectionDataSize(offset);

        assert(_TLSRegion.isPageAligned());
        assert(sourceLen % PAGE_SIZE == 0);

        for (size_t pageIndex = 0; pageIndex < _TLSRegion.length / PAGE_SIZE; pageIndex++) {
            PhysicalAddress physicalAddress = AllocatePhysicalFrame();
            /* if there is anything to be written to this page */
            if (sourceLen > pageIndex * PAGE_SIZE) {
                memcpy(reinterpret_cast<void *>(physicalAddress.value()), source, PAGE_SIZE);
            }

            if (pagingContext == PagingContext::GLOBAL) {
                MapAddress(pagingContext,
                           _TLSRegion.start + pageIndex * PAGE_SIZE,
                           physicalAddress,
                           true,
                           false,
                           false,
                           CacheType::CACHE_NORMAL_WRITE_BACK);
            } else {
                frames[frameIndex] = hos_v1::Frame{
                    .address = physicalAddress.value(),
                    .copyOnWriteCount = 1,
                };
                frameIndex++;
            }
        }

        _TLSBase = _TLSRegion.start + sectionAddress(offset) % PAGE_SIZE;
    }
}


