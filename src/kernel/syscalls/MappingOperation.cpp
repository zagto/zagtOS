#include <processes/Process.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>


static const int32_t PROTECTION_READ = 1,
                     PROTECTION_WRITE = 2,
                     PROTECTION_EXECUTE = 4;
static const uint32_t FLAG_SHARED = 0x01,
                      FLAG_PRIVATE = 0x02,
                      FLAG_FIXED = 0x10,
                      FLAG_ANONYMOUS = 0x20,
                      FLAG_PHYSICAL = 0x20000000;




bool MappingOperation::addressLengthValid() {
    return startAddress != 0
            && !(startAddress % PAGE_SIZE)
            && UserVirtualAddress::checkInRegion(startAddress)
            && startAddress + length > startAddress
            && length < UserSpaceRegion.end() - startAddress;
}


void MMap::perform(Process &process) {
    assert(process.pagingLock.isLocked());

    result = 0;
    error = 0;

    //cout << "MMAP addr " << start_address << " length " << length << " flags " << flags << " offset " << offset << endl;

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
        permissions = Permissions::READ;
    } else if (protection == (PROTECTION_WRITE | PROTECTION_READ)) {
        permissions = Permissions::READ_WRITE;
    } else if (protection == (PROTECTION_READ | PROTECTION_EXECUTE)) {
        permissions = Permissions::READ_EXECUTE;
    } else if (protection == 0) {
        permissions = Permissions::INVALID;
    } else {
        cout << "mmap: unsupported protection " << static_cast<uint64_t>(protection) << "\n";
        error = EINVAL;
        return;
    }

    Region passedRegion(startAddress, length);
    size_t insertIndex;
    Region actualRegion;
    bool slotReserved = false;

    if (addressLengthValid() && process.mappedAreas.isRegionFree(passedRegion, insertIndex)) {
        actualRegion = passedRegion;
    } else {
        /* can't use passed address/length */
        if (flags & FLAG_FIXED) {
            if (addressLengthValid()) {
                insertIndex = process.mappedAreas.unmapRange(passedRegion, 1);
                actualRegion = passedRegion;
                slotReserved = true;
            } else {
                cout << "MMAP: address " << startAddress << " length " << length
                     << " invalid and FIXED flag set" << endl;
                error = ENOMEM;
                return;
            }
        } else {
            bool valid;
            actualRegion = process.mappedAreas.findFreeRegion(length, valid, insertIndex);
            if (!valid) {
                cout << "MMAP: unable to auto-choose region\n";
                error = ENOMEM;
                return;
            }
        }
    }

    MappedArea *ma;
    if (protection == 0) {
        /* guard area */
        ma = new MappedArea(&process, actualRegion);
    } else if (flags & FLAG_ANONYMOUS) {
        ma = new MappedArea(&process, actualRegion, permissions);
    } else if (flags & FLAG_PHYSICAL) {
        ma = new MappedArea(&process, actualRegion, permissions, offset);
    } else {
        Panic();
    }

    if (slotReserved) {
        assert(process.mappedAreas[insertIndex] == nullptr);
        process.mappedAreas[insertIndex] = ma;
    } else {
        process.mappedAreas.insert2(ma, insertIndex);
    }
    result = ma->region.start;
}


void MProtect::perform(Process &process) {
    assert(process.pagingLock.isLocked());

    error = 0;

    length = align(length, PAGE_SIZE, AlignDirection::UP);
    if (length == 0) {
        cout << "MProtect of length 0" << endl;
        error = EINVAL;
        return;
    }

    if (!addressLengthValid()) {
        cout << "MProtect with invalid address " << startAddress << " length " << length << endl;
        error = EINVAL;
        return;
    }

    Permissions permissions;
    if (protection == PROTECTION_READ) {
        permissions = Permissions::READ;
    } else if (protection == (PROTECTION_WRITE | PROTECTION_READ)) {
        permissions = Permissions::READ_WRITE;
    } else if (protection == (PROTECTION_READ | PROTECTION_EXECUTE)) {
        permissions = Permissions::READ_EXECUTE;
    } else if (protection == 0) {
        permissions = Permissions::INVALID;
    } else {
        cout << "MProtect: unsupported protection " << static_cast<uint64_t>(protection) << "\n";
        error = EINVAL;
        return;
    }
    if (!process.mappedAreas.changeRangePermissions(Region(startAddress, length), permissions)) {
        cout << "MProtect at address " << startAddress << " length " << length
             << " which contains non-mapped pages" << endl;
        error = ENOMEM;
    }
}


void MUnmap::perform(Process &process) {
    assert(process.pagingLock.isLocked());

    error = 0;

    length = align(length, PAGE_SIZE, AlignDirection::UP);
    if (length == 0) {
        cout << "munmap of length 0" << endl;
        error = EINVAL;
        return;
    }

    if (!addressLengthValid()) {
        cout << "munmap with invalid address " << startAddress << " length " << length << endl;
        error = EINVAL;
        return;
    }

    process.mappedAreas.unmapRange(Region(startAddress, length));
}
