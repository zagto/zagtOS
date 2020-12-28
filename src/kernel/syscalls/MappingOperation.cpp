#include <processes/Process.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>


bool MappingOperation::addressLengthValid() {
    return startAddress != 0
            && !(startAddress % PAGE_SIZE)
            && UserVirtualAddress::checkInRegion(startAddress)
            && startAddress + length >= startAddress
            && length < UserSpaceRegion.end() - startAddress;
}


void MMap::perform(Process &process) {
    assert(process.pagingLock.isLocked());

    result = 0;
    error = 0;

    //cout << "MMAP addr " << startAddress << " length " << length << " flags " << flags << " offset " << offset << endl;

    if (offset % PAGE_SIZE != 0) {
        cout << "mmap offset not page-aligned" << endl;
        error = EINVAL;
        return;
    }

    if ((flags & MAP_WHOLE) && length != 0) {
        cout << "whole-area mmap with length parameter that is not 0" << endl;
        error = EINVAL;
        return;
    }
    if ((flags & MAP_WHOLE) && offset != 0) {
        cout << "whole-area mmap with offset parameter that is not 0" << endl;
        error = EINVAL;
        return;
    }
    if ((flags & MAP_WHOLE) && (flags & MAP_ANONYMOUS)) {
        cout << "mmap: both MAP_WHOLE and MAP_ANONYMOUS specified" << endl;
        error = EINVAL;
        return;
    }

    if (!(flags & MAP_WHOLE) && length == 0) {
        cout << "mmap of length 0" << endl;
        error = EINVAL;
        return;
    }

    if (!(flags & (MAP_SHARED | MAP_PRIVATE))) {
        cout << "mmap with neither SHARED nor PRIVATE flag set" << endl;
        error = EINVAL;
        return;
    }

    /* Shared and Private are the oposite of each other, they should not be used at the same time.
     * On Linux MAP_PRIVATE|MAP_SHARED means MAP_SHARED_VALIDATE, so treat them this the same as
     * MAP_SHARED. We don't offer not validating. */
    if ((flags & MAP_PRIVATE) && (flags & MAP_SHARED)) {
        flags = flags & ~(MAP_PRIVATE);
    }

    if ((flags & MAP_SHARED) && offset % PAGE_SIZE != 0) {
        cout << "mmap with MAP_SHARED set and non-aligned offset" << endl;
        error = EINVAL;
        return;
    }

    if ((flags & ~MAP_FIXED) != (MAP_PRIVATE | MAP_ANONYMOUS)
            && (flags & ~MAP_FIXED) != MAP_SHARED
            && (flags & ~MAP_FIXED) != (MAP_SHARED | MAP_WHOLE)) {
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
    shared_ptr<MemoryArea> memoryArea;

    if (!(flags & MAP_ANONYMOUS)) {
        optional<shared_ptr<MemoryArea>> temp = process.handleManager.lookupMemoryArea(handle);
        if (!temp) {
            cout << "MMAP: passed handle " << handle << " is not a valid MemoryArea object" << endl;
            error = EBADF;
            return;
        }
        memoryArea = *temp;
        if (!memoryArea->allowesPermissions(permissions)) {
            cout << "MMAP: requested permissions not allowed on this MemoryArea object" << endl;
            error = EACCESS;
            return;
        }
        if (flags & MAP_WHOLE) {
            length = memoryArea->length;
            passedRegion.length = memoryArea->length;
        } else {
            if (passedRegion.length + offset < passedRegion.length
                    || passedRegion.length + offset < memoryArea->length) {
                cout << "MMAP: requested range to big for this MemoryArea object" << endl;
                error = ENXIO;
                return;
            }
        }
    }

    size_t insertIndex;
    Region actualRegion;
    bool slotReserved = false;

    if (addressLengthValid() && process.mappedAreas.isRegionFree(passedRegion, insertIndex)) {
        actualRegion = passedRegion;
    } else {
        /* can't use passed address/length */
        if (flags & MAP_FIXED) {
            if (addressLengthValid()) {
                insertIndex = process.mappedAreas.unmapRange(passedRegion, 1);
                actualRegion = passedRegion;
                slotReserved = true;
            } else {
                cout << "MMAP: address " << startAddress << " length " << passedRegion.length
                     << " invalid and FIXED flag set" << endl;
                error = ENOMEM;
                return;
            }
        } else {
            bool valid;
            actualRegion = process.mappedAreas.findFreeRegion(passedRegion.length, valid, insertIndex);
            if (!valid) {
                cout << "MMAP: unable to auto-choose region\n";
                error = ENOMEM;
                return;
            }
        }
    }

    if (flags & MAP_ANONYMOUS) {
        memoryArea = make_shared<MemoryArea>(actualRegion.length);
    }

    MappedArea *ma = new MappedArea(&process, actualRegion, move(memoryArea), offset, permissions);
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

    if (!addressLengthValid()) {
        cout << "munmap with invalid address " << startAddress << " length " << length << endl;
        error = EINVAL;
        return;
    }

    if (wholeArea) {
        if (length != 0) {
            cout << "whole-area munmap with length parameter that is not 0" << endl;
            error = EINVAL;
            return;
        }

        MappedArea *area = process.mappedAreas.findMappedArea(startAddress);
        if (area == nullptr || area->region.start != startAddress) {
            cout << "whole-area munmap address not the beginning of an area" << endl;
            error = EINVAL;
            return;
        }
        length = area->region.length;
    }
    if (length == 0) {
        cout << "sized munmap of length 0" << endl;
        error = EINVAL;
        return;
    }

    process.mappedAreas.unmapRange(Region(startAddress, length));
}
