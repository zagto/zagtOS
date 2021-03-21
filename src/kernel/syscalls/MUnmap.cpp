#include <syscalls/MUnmap.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>

Result<size_t> MUnmap(const shared_ptr<Process> &process,
                      size_t wholeArea,
                      size_t startAddress,
                      size_t length,
                      size_t,
                      size_t) {
    assert(process->pagingLock.isLocked());

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    if (!addressLengthValid(startAddress, length)) {
        cout << "munmap with invalid address " << startAddress << " length " << length << endl;
        return EINVAL;
    }

    if (wholeArea) {
        if (length != 0) {
            cout << "whole-area munmap with length parameter that is not 0" << endl;
            /* whole-area unmap will crash on invalid input because it is ZagtOS-specific, so
             * unlike the rest of MUnmap, where code may depend on getting error codes. */
            return Status::BadUserSpace();
        }

        MappedArea *area = process->mappedAreas.findMappedArea(startAddress);
        if (area == nullptr || area->region.start != startAddress) {
            cout << "whole-area munmap address not the beginning of an area" << endl;
            return Status::BadUserSpace();
        }
        length = area->region.length;
    }
    if (length == 0) {
        cout << "sized munmap of length 0" << endl;
        return EINVAL;
    }

    Result result = process->mappedAreas.unmapRange(Region(startAddress, length));
    if (result) {
        return 0;
    } else {
        return result.status();
    }
}
