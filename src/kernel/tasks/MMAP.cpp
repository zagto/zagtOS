#include <tasks/Object.hpp>
#include <tasks/Task.hpp>
#include <tasks/MMAP.hpp>
#include <tasks/ErrorCodes.hpp>


static const i32 PROTECTION_READ = 1,
                 PROTECTION_WRITE = 2,
                 PROTECTION_EXECUTE = 4;
static const u32 FLAG_PRIVATE = 0x02,
                 FLAG_FIXED = 0x10,
                 FLAG_ANONYMOUS = 0x20;




bool MMAP::addressLengthValid() {
    return start_address
            && !(start_address % PAGE_SIZE)
            && UserVirtualAddress::checkInRegion(start_address)
            && start_address + length > start_address
            && UserVirtualAddress::checkInRegion(start_address + length);
}


void MMAP::perform(Task &task) {
    Assert(task.pagingLock.isLocked());

    result = 0;
    error = 0;

    if (!length) {
        error = EINVAL;
        return;
    }

    if (flags & FLAG_FIXED) {
        if (!addressLengthValid()) {
            error = ENOMEM;
            return;
        }
    } else {
       // if (!addressLengthValid() || !addressLengthAvailable()) {

       // }
    }

    Permissions permissions;
    if (protection == PROTECTION_READ) {
        permissions = Permissions::NONE;
    } else if (protection == (PROTECTION_WRITE | PROTECTION_READ)) {
        permissions = Permissions::WRITE;
    } else if (protection == (PROTECTION_READ | PROTECTION_EXECUTE)) {
        permissions = Permissions::EXECUTE;
    } else {
        Log << "mmap: unsupported protection " << static_cast<u64>(protection) << "\n";
        error = EINVAL;
        return;
    }

    Log << "TDODO: doMMAP" << static_cast<usize>(permissions == Permissions::WRITE) << "\n";
}
