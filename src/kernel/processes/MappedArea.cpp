#include <common/common.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>


MappedArea::MappedArea(Process *process,
                       Region region,
                       shared_ptr<MemoryArea> _memoryArea,
                       size_t offset,
                       Permissions permissions,
                       Status &status) :
        process{process},
        memoryArea{move(_memoryArea)},
        offset{offset},
        region{region},
        permissions{permissions} {
    if (!status) {
        return;
    }
    assert(UserVirtualAddress::checkInRegion(region.start));
    assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    assert(region.isPageAligned());
    assert(permissions != Permissions::READ_WRITE_EXECUTE);
    assert(memoryArea->allowesPermissions(permissions));
}

MappedArea::MappedArea(Process *process,
                       shared_ptr<MemoryArea> _memoryArea,
                       const hos_v1::MappedArea &handOver,
                       Status &) :
        process{process},
        memoryArea{move(_memoryArea)},
        region(handOver.start, handOver.length),
        permissions{handOver.permissions} {
    cout << "handover MappedArea " << handOver.start << " len " << handOver.length << endl;
}


MappedArea::~MappedArea() {
    unmapRange(region);
}


Status MappedArea::handlePageFault(UserVirtualAddress address) {
    assert(process->pagingLock.isLocked());
    assert(address.isInRegion(region));

    PagingContext *context = process->pagingContext;
    if (context->isMapped(address)) {
        context->invalidateLocally(address);
    } else {
        Result<PhysicalAddress> newFrame = memoryArea->makePresent(address.value() - region.start + offset);
        if (!newFrame) {
            return newFrame.status();
        }
        Status status = context->map(address, *newFrame, permissions, memoryArea->cacheType());
        if (!status) {
            return status;
        }
    }
    return Status::OK();
}

void MappedArea::unmapRange(Region range) {
    assert(range.isPageAligned());
    process->pagingContext->unmapRange(UserVirtualAddress(range.start),
                                       range.length / PAGE_SIZE,
                                       memoryArea->isAnonymous());
}

void MappedArea::shrinkFront(size_t amount) {
    assert(amount % PAGE_SIZE == 0);

    unmapRange(Region(region.start, amount));
    region.start += amount;
    region.length -= amount;
}

void MappedArea::shrinkBack(size_t amount) {
    assert(amount % PAGE_SIZE == 0);

    unmapRange(Region(region.end() - amount, amount));
    region.length -= amount;
}

void MappedArea::changePermissions(Permissions newPermissions) {
    assert(memoryArea->allowesPermissions(newPermissions));
    permissions = newPermissions;
}
