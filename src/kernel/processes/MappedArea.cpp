#include <common/common.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>
#include <memory/TLBContext.hpp>


MappedArea::MappedArea(ProcessAddressSpace &addressSpace,
                       Region region,
                       shared_ptr<MemoryArea> _memoryArea,
                       size_t offset,
                       Permissions permissions) :
        addressSpace{addressSpace},
        isPagedIn(region.length / PAGE_SIZE, false),
        memoryArea{move(_memoryArea)},
        offset{offset},
        region{region},
        permissions{permissions} {

    assert(UserVirtualAddress::checkInRegion(region.start));
    assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    assert(region.isPageAligned());
    assert(permissions != Permissions::READ_WRITE_EXECUTE);
    assert(memoryArea->allowesPermissions(permissions));
}

MappedArea::MappedArea(ProcessAddressSpace &addressSpace,
                       shared_ptr<MemoryArea> _memoryArea,
                       const hos_v1::MappedArea &handOver) :
        addressSpace{addressSpace},
        isPagedIn(handOver.length / PAGE_SIZE, false),
        memoryArea{move(_memoryArea)},
        offset{handOver.offset},
        region(handOver.start, handOver.length),
        permissions{handOver.permissions} {}


MappedArea::~MappedArea() {
    if (offset == static_cast<size_t>(-1)) {
        /* invilated area by split() method */
        assert(region.length == 1);
        assert(isPagedIn.size() == 0);
        return;
    }

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

void MappedArea::ensurePagedIn(UserVirtualAddress address) {
    assert(address.isInRegion(region));

    size_t pageIndex = (address.value() - region.start) / PAGE_SIZE;

    if (!isPagedIn[pageIndex]) {
        memoryArea->pageIn(addressSpace,
                           address,
                           permissions,
                           offset + pageIndex * PAGE_SIZE);
        isPagedIn[pageIndex] = true;
    }
}

pair<unique_ptr<MappedArea>, unique_ptr<MappedArea>> MappedArea::split(size_t splitOffset) {
    assert(splitOffset % PAGE_SIZE == 0);
    assert(splitOffset < region.length);

    unique_ptr<MappedArea> first, second;

    if (memoryArea->isShared) {
        /* shared MemoryAreas can't be split. Simply create two MappedAreas that reference the
         * same MemoryArea*/

        first = make_unique<MappedArea>(addressSpace,
                                        Region(region.start, splitOffset),
                                        memoryArea,
                                        offset,
                                        permissions);
        second = make_unique<MappedArea>(addressSpace,
                                         Region(region.start + splitOffset,
                                                region.length - splitOffset),
                                         memoryArea,
                                         offset + splitOffset,
                                         permissions);
    } else {
        /* Non-shared MemoryArea - split it! */

        auto memArea1 = make_shared<MemoryArea>(*memoryArea,
                                                offset,
                                                splitOffset);
        auto memArea2 = make_shared<MemoryArea>(*memoryArea,
                                                offset + splitOffset,
                                                region.length - splitOffset);

        first = make_unique<MappedArea>(addressSpace,
                                        Region(region.start, splitOffset),
                                        memArea1,
                                        0,
                                        permissions);
        second = make_unique<MappedArea>(addressSpace,
                                         Region(region.start + splitOffset,
                                                region.length - splitOffset),
                                         memArea2,
                                         0,
                                         permissions);
    }

    /* make this MappedArea invalid so it can not be accidently be used */
    memoryArea.reset();
    offset = -1;
    region = {static_cast<size_t>(-1), 1};
    isPagedIn = vector<bool>();

    return make_pair(move(first), move(second));
}
