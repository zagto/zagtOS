#include <syscalls/MProtect.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/ErrorCodes.hpp>

Result<size_t> MProtect(const shared_ptr<Process> &process,
                        size_t startAddress,
                        size_t length,
                        size_t protection,
                        size_t,
                        size_t) {
    length = align(length, PAGE_SIZE, AlignDirection::UP);
    if (length == 0) {
        cout << "MProtect of length 0" << endl;
        return EINVAL;
    }

    if (!addressLengthValid(startAddress, length)) {
        cout << "MProtect with invalid address " << startAddress << " length " << length << endl;
        return EINVAL;
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
        return EOPNOTSUPP;
    }
    if (!process->addressSpace.changeRegionProtection(Region(startAddress, length), permissions)) {
        cout << "MProtect at address " << startAddress << " length " << length
             << " which contains non-mapped pages" << endl;
        return ENOMEM;
    }
    return 0;
}
