#include <syscalls/SendMessage.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <memory/UserSpaceObject.hpp>

Result<size_t> SendMessage(const shared_ptr<Process> &process,
           uint64_t handle,
           uint64_t messageTypeAddress,
           uint64_t messageAddress,
           uint64_t messageSize,
           uint64_t numMessageHandles){
    Result<weak_ptr<Port>> weakPort = process->handleManager.lookupRemotePort(handle);
    if (!weakPort) {
        cout << "sendMessage: invalid port handle " << handle << endl;
        return weakPort.status();
    }

    shared_ptr<Port> port = weakPort->lock();
    if (!port) {
        cout << "sendMessage: destination port no longer exists: " << handle << endl;
        return ENXIO;
    }

    scoped_lock sl(process->pagingLock, port->process->pagingLock);
    if (!process->verifyMessageAccess(messageAddress, messageSize, numMessageHandles)) {
        Panic(); // TODO: exception
    }
    UserSpaceObject<UUID, USOOperation::READ> messageType(messageTypeAddress, process);
    if (!messageType.valid) {
        Panic(); // TODO: exception
    }

    unique_ptr<Message> message = make_unique<Message>(process.get(),
                                                       port->process.get(),
                                                       messageAddress,
                                                       messageType.object,
                                                       messageSize,
                                                       numMessageHandles);
    message->transfer();
    port->addMessage(move(message));
    return 0;
}
