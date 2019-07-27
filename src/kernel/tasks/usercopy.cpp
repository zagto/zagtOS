#include <system/System.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Task.hpp>


bool Task::accessUserSpace(u8 *buffer,
                           usize start,
                           usize length,
                           MasterPageTable::AccessOpertion accOp,
                           bool requireWritePermissions) {
    Assert(CurrentProcessor->currentTask->pagingLock.isLocked());

    if (!VirtualAddress(start).isInRegion(UserSpaceRegion)) {
        return false;
    }
    if (length > UserSpaceRegion.end() - start) {
        return false;
    }

    usize alignedStart = start;
    usize alignedLength = length;
    alignedGrow(alignedStart, alignedLength, PAGE_SIZE);

    MappedArea *area = mappedAreas.findMappedArea(UserVirtualAddress(alignedStart));
    if (area == nullptr) {
        Log << "accessUserSpace beginning at unmapped address " << start << "\n";
        return false;
    }

    usize areaPrefix = alignedStart - area->region.start;
    if (area->region.length - areaPrefix < alignedLength) {
        Log << "accessUserSpace address " << start << ", length " << length << " too long for "
            << "area " << area->region.start << ", length: " << area->region.length <<"\n";
        return false;
    }

    if (requireWritePermissions && area->permissions != Permissions::WRITE) {
        Log << "accessUserSpace address " << start << ": no write permissions\n";
        return false;
    }

    if (accOp != MasterPageTable::AccessOpertion::VERIFY_ONLY) {
        usize pagePrefix = start - alignedStart;

        Log << "aligned length: " << alignedLength << "\n";

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

bool Task::copyFromUser(u8 *destination, usize address, usize length, bool requireWritePermissions) {
    return accessUserSpace(destination,
                           address,
                           length,
                           MasterPageTable::AccessOpertion::READ,
                           requireWritePermissions);
}

bool Task::copyToUser(usize address, const u8 *source, usize length, bool requireWritePermissions) {
    return accessUserSpace(const_cast<u8 *>(source),
                           address,
                           length,
                           MasterPageTable::AccessOpertion::WRITE,
                           requireWritePermissions);

    u64 v1, v2;
    v1 = *((u64 *)source);
    v2 = *UserVirtualAddress(address).asPointer<u64>();
    Assert(v1 == v2);
    v1 = ((u64 *)source)[3];
    v2 = UserVirtualAddress(address).asPointer<u64>()[3];
    Assert(v1 == v2);
}

bool Task::verifyUserAccess(usize address, usize length, bool requireWritePermissions) {
    return accessUserSpace(nullptr,
                           address,
                           length,
                           MasterPageTable::AccessOpertion::VERIFY_ONLY,
                           requireWritePermissions);
}
