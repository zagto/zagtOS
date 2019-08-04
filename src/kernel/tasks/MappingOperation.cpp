#include <tasks/Object.hpp>
#include <tasks/Task.hpp>
#include <tasks/MappingOperation.hpp>
#include <tasks/ErrorCodes.hpp>


static const int32_t PROTECTION_READ = 1,
                     PROTECTION_WRITE = 2,
                     PROTECTION_EXECUTE = 4;
static const uint32_t FLAG_SHARED = 0x01,
                      FLAG_PRIVATE = 0x02,
                      FLAG_FIXED = 0x10,
                      FLAG_ANONYMOUS = 0x20,
                      FLAG_PHYSICAL = 0x20000000;




bool MappingOperation::addressLengthValid() {
    return start_address != 0
            && !(start_address % PAGE_SIZE)
            && UserVirtualAddress::checkInRegion(start_address)
            && start_address + length > start_address
            && length < UserSpaceRegion.end() - start_address;
}


void MMap::perform(Task &task) {
    assert(task.pagingLock.isLocked());

    result = 0;
    error = 0;

    cout << "MMAP addr " << start_address << " length " << length << " flags " << flags << " offset " << offset << endl;

    if (offset % PAGE_SIZE != 0) {
        cout << "mmap offset not page-aligned" << endl;
        error = EINVAL;
        return;
    }

    length = align(length, PAGE_SIZE, AlignDirection::UP);
    if (length == 0) {
        cout << "mmap of length 0" << endl;
        error = EINVAL;
        return;
    }

    if (!(flags & (FLAG_SHARED | FLAG_PRIVATE))) {
        cout << "mmap with neither SHARED nor PRIVATE flag set" << endl;
        error = EINVAL;
        return;
    }

    /* Shared and Private are the oposite of each other, they should not be used at the same time.
     * On Linux MAP_PRIVATE|MAP_SHARED means MAP_SHARED_VALIDATE, so treat them this the same as
     * MAP_PRIVATE. We don't offer not validating. */
    if ((flags & FLAG_PRIVATE) && (flags & FLAG_SHARED)) {
        flags = flags & ~(FLAG_PRIVATE);
    }

    /* Can't do anonymous and physical memory mapping at the same time */
    if ((flags & FLAG_ANONYMOUS) && (flags & FLAG_PHYSICAL)) {
        cout << "mmap both ANONYMOUS and PHYSICAL flag set" << endl;
        error = EINVAL;
        return;
    }

    if ((flags & FLAG_PHYSICAL) && offset % PAGE_SIZE != 0) {
        cout << "mmap with PHYSICAL set and non-aligned offset" << endl;
        error = EINVAL;
        return;
    }

    if ((flags & ~FLAG_FIXED) != (FLAG_PRIVATE | FLAG_ANONYMOUS)
            && (flags & ~FLAG_FIXED) != (FLAG_SHARED | FLAG_PHYSICAL)) {
        cout << "MMAP: unsupported flags: " << flags << endl;
        error = EOPNOTSUPP;
        return;
    }

    Permissions permissions;
    if (protection == PROTECTION_READ) {
        permissions = Permissions::NONE;
    } else if (protection == (PROTECTION_WRITE | PROTECTION_READ)) {
        permissions = Permissions::WRITE;
    } else if (protection == (PROTECTION_READ | PROTECTION_EXECUTE)) {
        permissions = Permissions::EXECUTE;
    } else {
        cout << "mmap: unsupported protection " << static_cast<uint64_t>(protection) << "\n";
        error = EINVAL;
        return;
    }

    Region passedRegion(start_address, length);
    size_t insertIndex;
    Region actualRegion;
    bool slotReserved = false;

    if (addressLengthValid() && task.mappedAreas.isRegionFree(passedRegion, insertIndex)) {
        actualRegion = passedRegion;
    } else {
        /* can't use passed address/length */
        if (flags & FLAG_FIXED) {
            if (addressLengthValid()) {
                insertIndex = task.mappedAreas.unmapRange(passedRegion, 1);
                actualRegion = passedRegion;
                slotReserved = true;
            } else {
                cout << "MMAP: address " << start_address << " length " << length
                     << " invalid and FIXED flag set" << endl;
                error = ENOMEM;
                return;
            }
        } else {
            bool valid;
            actualRegion = task.mappedAreas.findFreeRegion(length, valid, insertIndex);
            if (!valid) {
                cout << "MMAP: unable to auto-choose region\n";
                error = ENOMEM;
                return;
            }
        }
    }

    MappedArea *ma;
    if (flags & FLAG_ANONYMOUS) {
        ma = new MappedArea(&task, actualRegion, permissions);
    } else if (flags & FLAG_PHYSICAL) {
        ma = new MappedArea(&task, actualRegion, permissions, offset);
    } else {
        Panic();
    }

    if (slotReserved) {
        assert(task.mappedAreas[insertIndex] == nullptr);
        task.mappedAreas[insertIndex] = ma;
    } else {
        task.mappedAreas.insert2(ma, insertIndex);
    }
    result = ma->region.start;
}


void MUnmap::perform(Task &task) {
    assert(task.pagingLock.isLocked());

    error = 0;

    length = align(length, PAGE_SIZE, AlignDirection::UP);
    if (length == 0) {
        cout << "munmap of length 0" << endl;
        error = EINVAL;
        return;
    }

    if (!addressLengthValid()) {
        cout << "munmap with invalid address " << start_address << " length " << length << endl;
        error = EINVAL;
        return;
    }

    task.mappedAreas.unmapRange(Region(start_address, length));
}
