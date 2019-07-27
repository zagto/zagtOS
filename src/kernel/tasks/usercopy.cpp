#include <system/System.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Task.hpp>


bool Task::accessUserSpace(uint8_t *buffer,
                           size_t start,
                           size_t length,
                           MasterPageTable::AccessOpertion accOp,
                           bool requireWritePermissions) {
    Assert(CurrentProcessor->currentTask->pagingLock.isLocked());

    if (!VirtualAddress(start).isInRegion(UserSpaceRegion)) {
        return false;
    }
    if (length > UserSpaceRegion.end() - start) {
        return false;
    }

    size_t alignedStart = start;
    size_t alignedLength = length;
    alignedGrow(alignedStart, alignedLength, PAGE_SIZE);

    MappedArea *area = mappedAreas.findMappedArea(UserVirtualAddress(alignedStart));
    if (area == nullptr) {
        cout << "accessUserSpace beginning at unmapped address " << start << "\n";
        return false;
    }

    size_t areaPrefix = alignedStart - area->region.start;
    if (area->region.length - areaPrefix < alignedLength) {
        cout << "accessUserSpace address " << start << ", length " << length << " too long for "
            << "area " << area->region.start << ", length: " << area->region.length <<"\n";
        return false;
    }

    if (requireWritePermissions && area->permissions != Permissions::WRITE) {
        cout << "accessUserSpace address " << start << ": no write permissions\n";
        return false;
    }

    if (accOp != MasterPageTable::AccessOpertion::VERIFY_ONLY) {
        size_t pagePrefix = start - alignedStart;

        masterPageTable->accessRange(UserVirtualAddress(alignedStart),
                                     alignedLength / PAGE_SIZE,
                                     pagePrefix,
                                     alignedLength - length - pagePrefix,
                                     buffer,
                                     accOp,
                                     area->permissions);
    }

    return true;
}

bool Task::copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions) {
    return accessUserSpace(destination,
                           address,
                           length,
                           MasterPageTable::AccessOpertion::READ,
                           requireWritePermissions);
}

bool Task::copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions) {
    return accessUserSpace(const_cast<uint8_t *>(source),
                           address,
                           length,
                           MasterPageTable::AccessOpertion::WRITE,
                           requireWritePermissions);
}

bool Task::verifyUserAccess(size_t address, size_t length, bool requireWritePermissions) {
    return accessUserSpace(nullptr,
                           address,
                           length,
                           MasterPageTable::AccessOpertion::VERIFY_ONLY,
                           requireWritePermissions);
}
