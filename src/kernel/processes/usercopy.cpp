#include <system/CommonSystem.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>


Status Process::accessUserSpace(uint8_t *buffer,
                           size_t start,
                           size_t length,
                           PagingContext::AccessOperation accOp,
                           bool requireWritePermissions) {
    assert(pagingLock.isLocked());
    if (!VirtualAddress::checkInRegion(UserSpaceRegion, start)) {
        return Status::BadUserSpace();
    }
    if (length > UserSpaceRegion.end() - start) {
        return Status::BadUserSpace();
    }

    size_t alignedStart = start;
    size_t alignedLength = length;
    alignedGrow(alignedStart, alignedLength, PAGE_SIZE);

    MappedArea *area = mappedAreas.findMappedArea(UserVirtualAddress(alignedStart));
    if (area == nullptr) {
        cout << "accessUserSpace beginning at unmapped address " << start << "\n";
        return Status::BadUserSpace();
    }

    size_t areaPrefix = alignedStart - area->region.start;
    if (area->region.length - areaPrefix < alignedLength) {
        cout << "accessUserSpace address " << start << ", length " << length << " too long for "
            << "area " << area->region.start << ", length: " << area->region.length << endl;
        return Status::BadUserSpace();
    }

    if (!area->memoryArea->isAnonymous()) {
        cout << "area for accessUserSpace at address " << start
             << ", {" << area->region.start << ", length: " << area->region.length << "}"
             << " which is not a standard anonymous memory mapping, but type"
             << static_cast<uint64_t>(area->memoryArea->source) << endl;
        return Status::BadUserSpace();
    }

    if (requireWritePermissions && area->permissions != Permissions::READ_WRITE) {
        cout << "accessUserSpace address " << start << ": no write permissions\n";
        return Status::BadUserSpace();
    }

    if (accOp != PagingContext::AccessOperation::VERIFY_ONLY) {
        size_t pagePrefix = start - alignedStart;

        pagingContext->accessRange(UserVirtualAddress(alignedStart),
                                     alignedLength / PAGE_SIZE,
                                     pagePrefix,
                                     alignedLength - length - pagePrefix,
                                     buffer,
                                     accOp,
                                     area->permissions);
    }

    return Status::OK();
}

Status Process::copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions) {
    return accessUserSpace(destination,
                           address,
                           length,
                           PagingContext::AccessOperation::READ,
                           requireWritePermissions);
}

Status Process::copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions) {
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

Status Process::copyFromOhterUserSpace(size_t destinationAddress,
                                  Process &sourceProcess,
                                  size_t sourceAddress,
                                  size_t length,
                                  bool requireWriteAccessToDestination) {
    if (length == 0) {
        return Status::OK();
    }

    /* TODO: implement this without intermediate buffer */
    Status status;
    vector<uint8_t> buffer(length, status);
    if (!status) {
        return status;
    }

    status = sourceProcess.copyFromUser(&buffer[0], sourceAddress, length, false);
    if (!status) {
        return status;
    }

    status = copyToUser(destinationAddress, &buffer[0], length, requireWriteAccessToDestination);
    return status;
}
