#include <system/System.hpp>
#include <tasks/Task.hpp>
#include <memory/PlatformRegions.hpp>
#include <memory/UserSpaceWindow.h>


UserSpaceWindow::UserSpaceWindow(u64 address, u64 size, u64 alignment)
{
    Assert(CurrentProcessor->currentTask->pagingLock.isLocked());

    valid = false;
    if (!VirtualAddress(address).isInRegion(UserSpaceRegion)) {
        return;
    }
    if (size > UserSpaceRegion.end() - address) {
        return;
    }
    if (address % alignment) {
        return;
    }

    _size = size;
    pointer = UserVirtualAddress(address).asPointer<u8>();

    alignedGrow(&address, &size, PAGE_SIZE);
    usize numPages = size / PAGE_SIZE;
    MasterPageTable *mpt = CurrentProcessor->currentTask->masterPageTable;
    for (usize index = 0; index < numPages; index++) {
        if (!mpt->isMapped(UserVirtualAddress(address + index * PAGE_SIZE))) {
            return;
        }
    }
    valid = true;
}


u8 &UserSpaceWindow::operator[](usize index) {
    Assert(isValid());
    Assert(index < _size);
    return pointer[index];
}


bool UserSpaceWindow::isValid() {
    return valid;
}


usize UserSpaceWindow::size() {
    return _size;
}
