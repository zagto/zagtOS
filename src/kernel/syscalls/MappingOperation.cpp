#include <processes/Process.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>


bool addressLengthValid(size_t startAddress, size_t length) {
    return startAddress != 0
            && !(startAddress % PAGE_SIZE)
            && UserVirtualAddress::checkInRegion(startAddress)
            && startAddress + length >= startAddress
            && length < UserSpaceRegion.end() - startAddress;
}
