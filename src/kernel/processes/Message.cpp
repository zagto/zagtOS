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
bool Message::transfer() {
    /* a message should not be transferred more than once */
    assert(!transferred);
    /* destination process may be null at first, but must exist now (see setDestinationProcess)*/
    assert(destinationProcess);
    /* this function accesses both source and destination address space, so they must be locked */
    assert(!sourceProcess || sourceProcess->pagingLock.isLocked());
    assert(destinationProcess->pagingLock.isLocked());

    if (!prepareMemoryArea()) {
        return false;
    }
    transferMessageInfo();
    if (!transferHandles()) {
        goto fail;
    }

    if (!transferData()) {
        goto fail;
    }

    /* After transfer, the message object may last longer than the source Process. To ensure nobody
     * tries to do something stupid with the pointer, erase it. */
    sourceProcess = nullptr;

    transferred = true;
    return true;

fail:
    /* In case of failure, do not leave a MappedArea in the destination Process behind. */
    destinationProcess->mappedAreas.remove(messageArea);
    return false;
}

/* Adds a MappedArea to the destination Process where the message+info will be written to. */
bool Message::prepareMemoryArea() {
    if (numHandles * HANDLE_SIZE > numBytes) {
        cout << "Message transfer failed: Message too small to contain all handles" << endl;
        return false;
    }

    /* The message metadata needs to be passed along with the run message, so for our purposes the
     * required size is the combined size of both. */
    size_t messageRegionSize = numBytes + sizeof(UserMessageInfo);

    messageArea = destinationProcess->mappedAreas.addNew(messageRegionSize, Permissions::READ);
    if (messageArea == nullptr) {
        cout << "TODO: decide what should happen here (huge message -> kill source process?)" << endl;
        Panic();
    }
    return true;
}

/* Writes the UserMessageInfo structure in the destination process. */
void Message::transferMessageInfo() {
    UserMessageInfo msgInfo = {
        messageType,
        destinationAddress().value(),
        numBytes,
        numHandles,
        true,
    };

    destinationProcess->copyToUser(infoAddress().value(),
                                   reinterpret_cast<uint8_t *>(&msgInfo),
                                   sizeof(UserMessageInfo),
                                   false);
}

/* creates a handle on destination side for each resource that was sent with the message and writes
 * them into destination memory */
bool Message::transferHandles() {
    if (numHandles == 0) {
        return true;
    }
    /* The run message to the initial Process is sent by the kernel (i.e. sourceProcess is null),
     * but does not contain handles. */
    assert(sourceProcess);

    vector<uint32_t> handles(numHandles);
    bool success = sourceProcess->copyFromUser(reinterpret_cast<uint8_t *>(handles.data()),
                                               sourceAddress.value(),
                                               handlesSize(),
                                               false);
    assert(success);

    success = sourceProcess->handleManager.transferHandles(handles, destinationProcess->handleManager);
    if (!success) {
        cout << "Message transfer failed: some handles were invalid." << endl;
        return false;
    }

    success = destinationProcess->copyToUser(destinationAddress().value(),
                                             reinterpret_cast<uint8_t *>(handles.data()),
                                             handlesSize(),
                                             false);
    assert(success);
    return true;
}

/* Copies directly copyable message data from source to destination Process.
 * Returns true on success, false if source area is not accessible (not mapped) */
bool Message::transferData() {
    if (simpleDataSize() == 0) {
        return true;
    }
    /* The run message to the initial Process is sent by the kernel (i.e. sourceProcess is null),
     * but does not contain data. */
    assert(sourceProcess);

    /* the directly copyable data is after the handles */
    return destinationProcess->copyFromOhterUserSpace(destinationAddress().value() + handlesSize(),
                                                      *sourceProcess,
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
    return infoAddress().value() + sizeof(UserMessageInfo);
}

/* Allow setting destination process later on because on SpawnProcess syscalls it does not exist at
 * first */
void Message::setDestinationProcess(Process *const process) {
    assert(!transferred);
    destinationProcess = process;
}

/* Returns the address of the message info (UserMessageInfo class) in the destination address
 * space. */
UserVirtualAddress Message::infoAddress() const {
    return messageArea->region.start;
}
