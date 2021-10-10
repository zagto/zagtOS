#include <syscalls/SendMessage.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <syscalls/UserSpaceObject.hpp>

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

    Status status;
    UserSpaceObject<UUID, USOOperation::READ> messageType(messageTypeAddress, status);
    if (!status) {
        return status;
    }

    Result message = make_unique<Message>(process.get(),
                                          port->process.get(),
                                          messageAddress,
                                          messageType.object,
                                          messageSize,
                                          numMessageHandles);
    if (!message) {
        return message.status();
    }

    status = (*message)->transfer();
    if (!status) {
        return status;
    }

    scoped_lock sl(port->lock);
    status = port->addMessage(move(*message));
    if (status) {
        return 0;
    } else {
        return status;
    }
}
