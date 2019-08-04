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

    if (offset % PAGE_SIZE != 0) {
        cout << "mmap offset not page-aligned" << endl;
        error = EINVAL;
        return;
    }

    align(length, PAGE_SIZE, AlignDirection::UP);
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
        error = EINVAL;
        return;
    }

    if ((flags & ~FLAG_FIXED) != (FLAG_PRIVATE | FLAG_ANONYMOUS)
            && (flags & ~FLAG_FIXED) != (FLAG_SHARED | FLAG_PHYSICAL)) {
        cout << "MMAP: unsupported flags: " << flags << endl;
        error = EOPNOTSUPP;
        return;
    }

    cout << "MMAP addr " << start_address << " length " << length << " flags " << flags << endl;

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
    MappedArea *ma;

    if (addressLengthValid() && task.mappedAreas.isRegionFree(passedRegion, insertIndex)) {
        ma = new MappedArea(&task, passedRegion, permissions);
        task.mappedAreas.insert2(ma, insertIndex);
    } else {
        /* can't use passed address/length */
        if (flags & FLAG_FIXED) {
            cout << "MMAP: address " << start_address << " length " << length
                 << " not available, but FIXED flag set" << endl;
            /* TODO: shoud actually unmap here */
            error = ENOMEM;
            return;
        }

        ma = task.mappedAreas.addNew(length, permissions);
        if (ma == nullptr) {
            cout << "MMAP: unable to auto-choose mapped area\n";
            error = ENOMEM;
            return;
        }
    }

    result = ma->region.start;
}


void MUnmap::perform(Task &task) {
    assert(task.pagingLock.isLocked());
    cout << "munmap !!!\n";

    error = 0;

    align(length, PAGE_SIZE, AlignDirection::UP);
    if (length == 0) {
        cout << "munmap of length 0" << endl;
        error = EINVAL;
        return;
    }

    if (!addressLengthValid()) {
        cout << "munmap with invalid address/length" << endl;
        error = EINVAL;
        return;
    }

    task.mappedAreas.unmapRange(Region(start_address, length));
}
