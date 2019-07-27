#include <tasks/Object.hpp>
#include <tasks/Task.hpp>
#include <tasks/MMAP.hpp>
#include <tasks/ErrorCodes.hpp>


static const int32_t PROTECTION_READ = 1,
                 PROTECTION_WRITE = 2,
                 PROTECTION_EXECUTE = 4;
static const uint32_t FLAG_PRIVATE = 0x02,
                 FLAG_FIXED = 0x10,
                 FLAG_ANONYMOUS = 0x20;




bool MMAP::addressLengthValid() {
    return start_address != 0
            && !(start_address % PAGE_SIZE)
            && UserVirtualAddress::checkInRegion(start_address)
            && start_address + length > start_address
            && length < UserSpaceRegion.end() - start_address;
}


void MMAP::perform(Task &task) {
    assert(task.pagingLock.isLocked());

    result = 0;
    error = 0;

    if (length == 0) {
        error = EINVAL;
        return;
    }

    if ((flags & ~FLAG_FIXED) != (FLAG_PRIVATE | FLAG_ANONYMOUS)) {
        cout << "MMAP: unsupported flags: " << flags << endl;
        error = EINVAL;
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
