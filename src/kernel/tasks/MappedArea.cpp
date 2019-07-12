#include <common/common.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Task.hpp>


MappedArea::MappedArea(Task *task, Region region, Permissions permissions) :
        task{task},
        source{Source::MEMORY},
        permissions{permissions},
        region{region} {
    Assert(UserVirtualAddress::checkInRegion(region.start));
    Assert(UserVirtualAddress::checkInRegion(region.end()));
    Assert(region.isPageAligned());
    Assert(permissions != Permissions::WRITE_AND_EXECUTE);
}


bool MappedArea::handlePageFault(UserVirtualAddress address) {
    if (source == Source::MEMORY) {
        Assert(task->pagingLock.isLocked());
        if (task->masterPageTable->isMapped(address)) {
            task->masterPageTable->invalidateLocally(address);
        } else {
            task->allocateFrame(address, permissions);
        }
        return true;
    } else {
        Panic();
    }
}

void MappedArea::mapEverything() {
    Assert(source == Source::MEMORY);

    for (usize address = region.start; address < region.length; address += PAGE_SIZE) {
        if (!task->masterPageTable->isMapped(address)) {
            task->masterPageTable->invalidateLocally(UserVirtualAddress(address));
        }
    }
}
