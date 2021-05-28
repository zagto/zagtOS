#include <common/common.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>
#include <memory/TLBContext.hpp>


MappedArea::MappedArea(ProcessAddressSpace &addressSpace,
                       Region region,
                       shared_ptr<MemoryArea> _memoryArea,
                       size_t offset,
                       Permissions permissions,
                       Status &status) :
        addressSpace{addressSpace},
        isPagedIn(region.length / PAGE_SIZE, false, status),
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

MappedArea::MappedArea(ProcessAddressSpace &addressSpace,
                       shared_ptr<MemoryArea> _memoryArea,
                       const hos_v1::MappedArea &handOver,
                       Status &status) :
        addressSpace{addressSpace},
        isPagedIn(handOver.length / PAGE_SIZE, false, status),
        memoryArea{move(_memoryArea)},
        offset{handOver.offset},
        region(handOver.start, handOver.length),
        permissions{handOver.permissions} {

    if (!status) {
        return;
    }
    cout << "handover MappedArea " << handOver.start << " len " << handOver.length << endl;
}


MappedArea::~MappedArea() {
    PageOutContext pageOut = pageOutRegion(region);
    pageOut.realize();
}

PageOutContext MappedArea::pageOutRegion(Region removeRegion) {
    assert(region.contains(removeRegion));
    assert(removeRegion.isPageAligned());

    PageOutContext context;

    size_t offsetInThis = (removeRegion.start - region.start);
    size_t offsetInMemArea = offsetInThis + offset;
    for (size_t pageIndex = 0; pageIndex < removeRegion.length / PAGE_SIZE; pageIndex++) {
        if (isPagedIn[pageIndex + offsetInThis / PAGE_SIZE]) {
            context |= memoryArea->pageOut(addressSpace,
                                           removeRegion.start + pageIndex * PAGE_SIZE,
                                           offsetInMemArea + pageIndex * PAGE_SIZE);
        }
    }
    return context;
}

Status MappedArea::ensurePagedIn(UserVirtualAddress address) {
    assert(address.isInRegion(region));

    size_t pageIndex = (address.value() - region.start) / PAGE_SIZE;

    if (!isPagedIn[pageIndex]) {
        Status status = memoryArea->pageIn(addressSpace,
                                           address,
                                           permissions,
                                           offset + pageIndex * PAGE_SIZE);
        if (!status) {
            return status;
        }
        isPagedIn[pageIndex] = true;
    }
    return Status::OK();
}

