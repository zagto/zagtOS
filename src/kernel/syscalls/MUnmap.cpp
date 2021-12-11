#include <syscalls/MUnmap.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>

size_t MUnmap(const shared_ptr<Process> &process,
                      size_t wholeArea,
                      size_t startAddress,
                      size_t length,
                      size_t,
                      size_t) {
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
            throw BadUserSpace(process);
        }

        process->addressSpace.removeMapping(startAddress);
    } else {
        if (length == 0) {
            cout << "sized munmap of length 0" << endl;
            return EINVAL;
        }

        process->addressSpace.removeRegion(Region(startAddress, length));
    }
    return 0;
}
