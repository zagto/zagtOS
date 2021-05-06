#include <common/common.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>
#include <memory/TLBContext.hpp>


MappedArea::MappedArea(Region region,
                       shared_ptr<MemoryArea> _memoryArea,
                       size_t offset,
                       Permissions permissions,
                       Status &status) :
        memoryArea{move(_memoryArea)},
        offset{offset},
        region{region},
        permissions{permissions},
        isPagedIn(region.length / PAGE_SIZE, false, status) {
    if (!status) {
        return;
    }
    assert(UserVirtualAddress::checkInRegion(region.start));
    assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    assert(region.isPageAligned());
    assert(permissions != Permissions::READ_WRITE_EXECUTE);
    assert(memoryArea->allowesPermissions(permissions));
}

MappedArea::MappedArea(shared_ptr<MemoryArea> _memoryArea,
                       const hos_v1::MappedArea &handOver,
                       Status &status) :
        memoryArea{move(_memoryArea)},
        offset{handOver.offset},
        region(handOver.start, handOver.length),
        permissions{handOver.permissions},
        isPagedIn(region.length / PAGE_SIZE, false, status) {

    if (!status) {
        return;
    }
    cout << "handover MappedArea " << handOver.start << " len " << handOver.length << endl;
}


MappedArea::~MappedArea() {
    unmapRange(region);
}


Status MappedArea::ensurePagedIn(TLBContext &tlbContext, UserVirtualAddress address) {
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
