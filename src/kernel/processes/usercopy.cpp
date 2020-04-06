#include <system/CommonSystem.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>


bool Process::accessUserSpace(uint8_t *buffer,
                           size_t start,
                           size_t length,
                           PagingContext::AccessOperation accOp,
                           bool requireWritePermissions) {
    assert(pagingLock.isLocked());

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

    if (accOp != PagingContext::AccessOperation::VERIFY_ONLY) {
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

bool Process::copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions) {
    return accessUserSpace(destination,
                           address,
                           length,
                           PagingContext::AccessOperation::READ,
                           requireWritePermissions);
}

bool Process::copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions) {
    return accessUserSpace(const_cast<uint8_t *>(source),
                           address,
                           length,
                           PagingContext::AccessOperation::WRITE,
                           requireWritePermissions);
}

bool Process::verifyUserAccess(size_t address, size_t length, bool requireWritePermissions) {
    return accessUserSpace(nullptr,
                           address,
                           length,
                           PagingContext::AccessOperation::VERIFY_ONLY,
                           requireWritePermissions);
}

bool Process::copyFromOhterUserSpace(size_t destinationAddress,
                                  Process &sourceProcess,
                                  size_t sourceAddress,
                                  size_t length,
                                  bool requireWriteAccessToDestination) {
    /* TODO: implement this without immideate buffer */
    vector<uint8_t> buffer(length);
    bool valid;

    valid = sourceProcess.copyFromUser(&buffer[0], sourceAddress, length, false);
    if (!valid) {
        return false;
    }

    valid = copyToUser(destinationAddress, &buffer[0], length, requireWriteAccessToDestination);
    return valid;
}
