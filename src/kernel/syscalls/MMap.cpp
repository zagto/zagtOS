#include <syscalls/MMap.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <syscalls/UserSpaceObject.hpp>

struct MMapStruct {
    size_t startAddress;
    size_t length;
    uint32_t flags;
    size_t offset;
    size_t result;
    uint32_t handle;
    uint32_t protection;

    Result<size_t> perform(const shared_ptr<Process> &process);
};

Result<size_t> MMapStruct::perform(const shared_ptr<Process> &process) {
    result = 0;

    //cout << "MMAP addr " << startAddress << " length " << length << " flags " << flags << " offset " << offset << endl;

    if (offset % PAGE_SIZE != 0) {
        cout << "mmap offset not page-aligned" << endl;
        return EINVAL;
    }

    if ((flags & MAP_WHOLE) && length != 0) {
        cout << "whole-area mmap with length parameter that is not 0" << endl;
        return EINVAL;
    }
    if ((flags & MAP_WHOLE) && offset != 0) {
        cout << "whole-area mmap with offset parameter that is not 0" << endl;
        return EINVAL;
    }
    if ((flags & MAP_WHOLE) && (flags & MAP_ANONYMOUS)) {
        cout << "mmap: both MAP_WHOLE and MAP_ANONYMOUS specified" << endl;
        return EINVAL;
    }

    if (!(flags & MAP_WHOLE) && length == 0) {
        cout << "mmap of length 0" << endl;
        return EINVAL;
    }

    if (!(flags & (MAP_SHARED | MAP_PRIVATE))) {
        cout << "mmap with neither SHARED nor PRIVATE flag set" << endl;
        return EINVAL;
    }

    /* Shared and Private are the oposite of each other, they should not be used at the same time.
     * On Linux MAP_PRIVATE|MAP_SHARED means MAP_SHARED_VALIDATE, so treat them this the same as
     * MAP_SHARED. We don't offer not validating. */
    if ((flags & MAP_PRIVATE) && (flags & MAP_SHARED)) {
        flags = flags & ~(MAP_PRIVATE);
    }

    if ((flags & MAP_SHARED) && offset % PAGE_SIZE != 0) {
        cout << "mmap with MAP_SHARED set and non-aligned offset" << endl;
        return EINVAL;
    }

    if ((flags & ~MAP_FIXED) != (MAP_PRIVATE | MAP_ANONYMOUS)
            && (flags & ~MAP_FIXED) != MAP_SHARED
            && (flags & ~MAP_FIXED) != (MAP_SHARED | MAP_WHOLE)) {
        cout << "MMAP: unsupported flags: " << flags << endl;
        return EOPNOTSUPP;
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
        return EINVAL;
    }

    Region passedRegion(startAddress, length);
    shared_ptr<MemoryArea> memoryArea;

    if (!(flags & MAP_ANONYMOUS)) {
        Result<shared_ptr<MemoryArea>> temp = process->handleManager.lookupMemoryArea(handle);
        if (!temp) {
            assert(temp.status() == Status::BadUserSpace());
            cout << "MMAP: passed handle " << handle << " is not a valid MemoryArea object" << endl;
            return EBADF;
        }
        memoryArea = *temp;
        if (!memoryArea->allowesPermissions(permissions)) {
            cout << "MMAP: requested permissions not allowed on this MemoryArea object" << endl;
            return EACCESS;
        }
        if (flags & MAP_WHOLE) {
            length = memoryArea->length;
            passedRegion.length = memoryArea->length;
        } else {
            if (passedRegion.length + offset < passedRegion.length
                    || passedRegion.length + offset < memoryArea->length) {
                cout << "MMAP: requested range to big for this MemoryArea object" << endl;
                return ENXIO;
                return Status::OK();
            }
        }
    }

    size_t insertIndex;
    Region actualRegion;
    bool slotReserved = false;

    scoped_lock sl(process->pagingLock);

    if (addressLengthValid(startAddress, length)
            && process->mappedAreas.isRegionFree(passedRegion, insertIndex)) {
        actualRegion = passedRegion;
    } else {
        /* can't use passed address/length */
        if (flags & MAP_FIXED) {
            process->addressSpace.addAnonymous(passedRegion, permissions, true);
            if (addressLengthValid(startAddress, length)) {
                /* MAP_FIXED set and passed region not free - we free it */
                insertIndex = process->mappedAreas.unmapRange(passedRegion, 1);
                actualRegion = passedRegion;
                slotReserved = true;
            } else {
                cout << "MMAP: address " << startAddress << " length " << passedRegion.length
                     << " invalid and FIXED flag set" << endl;
                return ENOMEM;
            }
        } else {
            bool valid;
            actualRegion = process->mappedAreas.findFreeRegion(passedRegion.length, valid, insertIndex);
            if (!valid) {
                cout << "MMAP: unable to auto-choose region\n";
                return ENOMEM;
            }
        }
    }

    if (flags & MAP_ANONYMOUS) {
        Result result = make_shared<MemoryArea>(actualRegion.length);
        if (!result) {
            return result.status();
        }
        memoryArea = *result;
    }

    Result<MappedArea *> ma = make_raw<MappedArea>(process.get(), actualRegion, move(memoryArea), offset, permissions);
    if (!ma) {
        return ma.status();
    }

    if (slotReserved) {
        assert(process->mappedAreas[insertIndex] == nullptr);
        process->mappedAreas[insertIndex] = *ma;
    } else {
        process->mappedAreas.insert2(*ma, insertIndex);
    }
    result = (*ma)->region.start;
    return 0;
}

Result<size_t> MMap(const shared_ptr<Process> &process,
                    size_t structAddress,
                    size_t,
                    size_t,
                    size_t,
                    size_t) {
    Status status = Status::OK();
    UserSpaceObject<MMapStruct, USOOperation::READ_AND_WRITE> uso(structAddress, status);
    if (!status) {
        if (status == Status::BadUserSpace()) {
            cout << "SYS_MMAP: process passed non-accessible regions as parameters structure" << endl;
        }
        return status;
    }

    Result result = uso.object.perform(process);
    if (!result) {
        return result.status();
    }

    status = uso.writeOut();
    if (status) {
        return *result;
    } else {
        return status;
    }
}

