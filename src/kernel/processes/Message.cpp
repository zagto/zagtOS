#include <vector>
#include <processes/Message.hpp>
#include <processes/Process.hpp>
#include <processes/UserApi.hpp>
#include <lib/Exception.hpp>


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
void Message::transfer() {
    /* a message should not be transferred more than once */
    assert(!transferred);
    /* destination process may be null at first, but must exist now (see setDestinationProcess)*/
    assert(destinationProcess);

    /* TODO: find a way to deal with bad user space here. It currently simply kills the source thread */

    prepareMemoryArea();

    try {
        transferMessageInfo();
        transferData();
        /* handles transfer should happen last - if something after it would fail, we leak the handles
         * in destination process */
        transferHandles();
    }  catch (...) {
        /* In case of failure, do not leave a MappedArea in the destination Process behind. */
        try {
            destinationProcess->addressSpace.removeMapping(infoAddress.value());
        } catch (BadUserSpace &e) {
            /* removeMapping only fails if given an invalid startAddress. This means the process would
             * have already unmapped the mapping by itself */
            cout << "Message::transfer: stupid Process unmapped message mapping by itself before even "
                 << "getting the address" << endl;
        }
        throw;
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
}

/* Adds a MappedArea to the destination Process where the message+info will be written to. */
void Message::prepareMemoryArea() {
    if (numHandles * HANDLE_SIZE > numBytes) {
        cout << "Message transfer failed: Message too small to contain all handles" << endl;
        throw BadUserSpace(sourceProcess->self.lock());
    }

    /* The message metadata needs to be passed along with the run message, so for our purposes the
     * required size is the combined size of both. */
    size_t messageRegionSize = numBytes + infoStructSize();

    try {
        /* Holds the address of the message info (ZoMessageInfo class) in the destination address
         * space. */
        infoAddress = destinationProcess->addressSpace.addAnonymous(messageRegionSize,
                                                                    Permissions::READ_WRITE);
    } catch (...) {
        cout << "TODO: decide what should happen here (huge message -> kill source process?)" << endl;
        throw;
    }
}

/* Writes the UserMessageInfo structure in the destination process. */
void Message::transferMessageInfo() {
    userApi::ZoMessageInfo msgInfo = {
        0, /* filled in later */
        messageType,
        reinterpret_cast<uint8_t*>(destinationAddress().value()),
        numBytes,
        numHandles,
        true,
    };

   if (isRunMessage) {
       userApi::ZoProcessStartupInfo startupInfo {
           threadHandle,
           messageQueueHandle,
           msgInfo,
       };
       return destinationProcess->addressSpace.copyTo(infoAddress.value(),
                                                      reinterpret_cast<uint8_t *>(&startupInfo),
                                                      sizeof(userApi::ZoProcessStartupInfo),
                                                      false);
   } else {
       return destinationProcess->addressSpace.copyTo(infoAddress.value(),
                                                      reinterpret_cast<uint8_t *>(&msgInfo),
                                                      sizeof(userApi::ZoMessageInfo),
                                                      false);
   }
}

/* creates a handle on destination side for each resource that was sent with the message and writes
 * them into destination memory */
void Message::transferHandles() {
    if (numHandles == 0) {
        return;
    }
    /* The run message to the initial Process is sent by the kernel (i.e. sourceProcess is null),
     * but does not contain handles. */
    assert(sourceProcess);

    vector<uint32_t> handles(numHandles);
    sourceProcess->addressSpace.copyFrom(reinterpret_cast<uint8_t *>(handles.data()),
                                         sourceAddress.value(),
                                         handlesSize());

    sourceProcess->handleManager.transferHandles(handles, destinationProcess->handleManager);

    try {
        destinationProcess->addressSpace.copyTo(destinationAddress().value(),
                                                reinterpret_cast<uint8_t *>(handles.data()),
                                                handlesSize(),
                                                false);
    } catch (...) {
        /* undo creating destination handles */
        for (uint32_t handle: handles) {
            shared_ptr<Thread> dummy;
            try {
                destinationProcess->handleManager.removeHandle(handle, dummy);
            } catch(BadUserSpace &e) {
                /* removing handles should only fail if invalid handle numbers are passed */
                cout << "transferHandles: destination process has messed with handles that were "
                     << "not even passed to it yet and deserves to crash." << endl;
            }
        }
        throw;
    }
}

/* Copies directly copyable message data from source to destination Process.
 * Returns true on success, false if source area is not accessible (not mapped) */
void Message::transferData() {
    if (simpleDataSize() == 0) {
        return;
    }
    /* the directly copyable data is after the handles */
    destinationProcess->addressSpace.copyFromOhter(destinationAddress().value() + handlesSize(),
                                                   sourceProcess->addressSpace,
                                                   sourceAddress.value() + handlesSize(),
                                                   simpleDataSize(),
                                                   false);
}

size_t Message::infoStructSize() const noexcept {
    if (isRunMessage) {
        return sizeof(userApi::ZoProcessStartupInfo);
    } else {
        return sizeof(userApi::ZoMessageInfo);
    }
}

size_t Message::handlesSize() const noexcept {
    return numHandles * HANDLE_SIZE;
}

size_t Message::simpleDataSize() const noexcept {
    return numBytes - handlesSize();
}

/* Returns the address of the message itself (not message info) in the destination address space. */
UserVirtualAddress Message::destinationAddress() const noexcept {
    /* Message follows directly after message info. */
    return infoAddress.value() + infoStructSize();
}

/* Allow setting destination process later on because on SpawnProcess syscalls it does not exist at
 * first */
void Message::setDestinationProcess(Process *const process) noexcept {
    assert(!transferred);
    destinationProcess = process;
}

void Message::setStartupInfo(uint32_t threadHandle, uint32_t messageQueueHandle) noexcept {
    assert(isRunMessage);
    this->threadHandle = threadHandle;
    this->messageQueueHandle = messageQueueHandle;
}
