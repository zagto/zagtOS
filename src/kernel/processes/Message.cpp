#include <vector>
#include <processes/Message.hpp>
#include <processes/Process.hpp>


/* Transfers the message into the destination address space.
 * Returns true on success, false if the source data is invalid.
 * The resulting data in the destination process will consist of 3 parts:
 *  - UserMessageInfo structure with meta information the process needs to understand the message
 *  - Handles: these will be translated from the source process' handle namespace to destination.
 *    Sending a handle in a message always gives the destination process access to the resource
 *    described by the handle.
 *  - Simple Data.
 * The second and third part are read from the source address space. Meta-information is passed by
 * the source Process using the syscall it used to send the Message and was given to this object via
 * the constructor.
 *
 *                        source                                destination
 *                                                          +-----------------+ <- infoAddress
 *                                                          | UserMessageInfo |
 * sourceAddress -> +-----------------+                     |-----------------| <- destinationAddress
 *                  | handles         | -- translation -->  | handles         |
 *                  |-----------------|                     |-----------------|
 *                  | simple data     | --     copy    -->  | simple data     |
 *                  +-----------------+                     +-----------------+
 */
Status Message::transfer() {
    /* a message should not be transferred more than once */
    assert(!transferred);
    /* destination process may be null at first, but must exist now (see setDestinationProcess)*/
    assert(destinationProcess);

    Status status = prepareMemoryArea();
    if (!status) {
        return status;
    }

    status = transferMessageInfo();
    if (!status) {
        goto fail;
    }

    status = transferData();
    if (!status) {
        goto fail;
    }

    /* handles transfer should happen last - if something after it would fail, we leak the handles
     * in destination process */
    status = transferHandles();
    if (!status) {
        goto fail;
    }

    /* After transfer, multiple fields are no longer needed. Clearing them helps us with 2 things:
     * - The message object may last longer than the source Process, (or in theory messageArea if
     *   destination munmaps it before receiving for some reason). We need to ensure nobody tries to
     *   do something stupid with the pointers.
     * - During kernel handover all messages are transferred completely, which means we can have a
     *   much simpler handover interface by excluding these fields */
    sourceProcess = nullptr;
    destinationProcess = nullptr;
    sourceAddress = {};
    messageType = {0};
    numBytes = 0;
    numHandles = 0;

    transferred = true;
    return Status::OK();

fail:
    /* In case of failure, do not leave a MappedArea in the destination Process behind. */
    destinationProcess->addressSpace.removeMapping(infoAddress.value());
    return status;
}

/* Adds a MappedArea to the destination Process where the message+info will be written to. */
Status Message::prepareMemoryArea() {
    if (numHandles * HANDLE_SIZE > numBytes) {
        cout << "Message transfer failed: Message too small to contain all handles" << endl;
        return BadUserSpace;
    }

    /* The message metadata needs to be passed along with the run message, so for our purposes the
     * required size is the combined size of both. */
    size_t messageRegionSize = numBytes + sizeof(UserMessageInfo);

    Result result = destinationProcess->addressSpace.addAnonymous(messageRegionSize,
                                                                  Permissions::READ);
    if (!result) {
        cout << "TODO: decide what should happen here (huge message -> kill source process?)" << endl;
        return result.status();
    }

    /* Holds the address of the message info (UserMessageInfo class) in the destination address
     * space. */
    infoAddress = result->start;
    return Status::OK();
}

/* Writes the UserMessageInfo structure in the destination process. */
Status Message::transferMessageInfo() {
    UserMessageInfo msgInfo = {
        messageType,
        destinationAddress().value(),
        numBytes,
        numHandles,
        true,
    };

   return destinationProcess->addressSpace.copyTo(infoAddress.value(),
                                                  reinterpret_cast<uint8_t *>(&msgInfo),
                                                  sizeof(UserMessageInfo),
                                                  false);
}

/* creates a handle on destination side for each resource that was sent with the message and writes
 * them into destination memory */
Status Message::transferHandles() {
    if (numHandles == 0) {
        return Status::OK();
    }
    /* The run message to the initial Process is sent by the kernel (i.e. sourceProcess is null),
     * but does not contain handles. */
    assert(sourceProcess);

    Status status;
    vector<uint32_t> handles(numHandles, status);
    if (!status) {
        return status;
    }
    status = sourceProcess->addressSpace.copyFrom(reinterpret_cast<uint8_t *>(handles.data()),
                                                  sourceAddress.value(),
                                                  handlesSize());
    if (!status) {
        return status;
    }

    status = sourceProcess->handleManager.transferHandles(handles, destinationProcess->handleManager);
    if (!status) {
        cout << "Message transfer failed: some handles were invalid." << endl;
        return status;
    }

    status = destinationProcess->addressSpace.copyTo(destinationAddress().value(),
                                                     reinterpret_cast<uint8_t *>(handles.data()),
                                                     handlesSize(),
                                                     false);
    if (!status) {
        /* undo creating destination handles */
        for (uint32_t handle: handles) {
            shared_ptr<Thread> dummy;
            Status removeSuccess = destinationProcess->handleManager.removeHandle(handle, dummy);
            if (!removeSuccess || dummy) {
                cout << "transferHandles: destination process has messed with handles that were "
                     << "not even passed to it yet and deserves to crash." << endl;
                /* removing handles should only fail if invalid handle numbers are passed */
                assert(removeSuccess == Status::BadUserSpace());
            }
        }
    }
    return status;
}

/* Copies directly copyable message data from source to destination Process.
 * Returns true on success, false if source area is not accessible (not mapped) */
Status Message::transferData() {
    if (simpleDataSize() == 0) {
        return Status::OK();
    }
    /* The run message to the initial Process is sent by the kernel (i.e. sourceProcess is null),
     * but does not contain data. */
    assert(sourceProcess);

    /* the directly copyable data is after the handles */
    return destinationProcess->addressSpace.copyFromOhter(destinationAddress().value() + handlesSize(),
                                                          sourceProcess->addressSpace,
                                                          sourceAddress.value() + handlesSize(),
                                                          simpleDataSize(),
                                                          false);
}

size_t Message::handlesSize() const {
    return numHandles * HANDLE_SIZE;
}

size_t Message::simpleDataSize() const {
    return numBytes - handlesSize();
}

/* Returns the address of the message itself (not message info) in the destination address space. */
UserVirtualAddress Message::destinationAddress() const {
    /* Message follows directly after message info. */
    return infoAddress.value() + sizeof(UserMessageInfo);
}

/* Allow setting destination process later on because on SpawnProcess syscalls it does not exist at
 * first */
void Message::setDestinationProcess(Process *const process) {
    assert(!transferred);
    destinationProcess = process;
}

