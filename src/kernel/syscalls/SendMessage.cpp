#include <syscalls/SendMessage.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <syscalls/UserSpaceObject.hpp>

size_t SendMessage(const shared_ptr<Process> &process,
           uint64_t handle,
           uint64_t messageTypeAddress,
           uint64_t messageAddress,
           uint64_t messageSize,
           uint64_t numMessageHandles){
    weak_ptr<Port> weakPort = process->handleManager.lookupRemotePort(handle);
    shared_ptr<Port> port = weakPort.lock();
    if (!port) {
        cout << "sendMessage: destination port no longer exists: " << handle << endl;
        return ENXIO;
    }

    UserSpaceObject<UUID, USOOperation::READ> messageType(messageTypeAddress);

    auto message = make_unique<Message>(process.get(),
                                        port->process.get(),
                                        messageAddress,
                                        messageType.object,
                                        messageSize,
                                        numMessageHandles);
    message->transfer();

    scoped_lock sl(port->lock);
    port->addMessage(move(message));
    return 0;
}
