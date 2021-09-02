#include <syscalls/ReceiveMessage.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>

Result<size_t> ReceiveMessage(const shared_ptr<Process> &process,
                   size_t portHandle,
                   size_t,
                   size_t,
                   size_t,
                   size_t) {
    Result<shared_ptr<Port>> port = process->handleManager.lookupPort(static_cast<uint32_t>(portHandle));
    if (!port) {
        cout << "receiveMessage: invalid port handle " << portHandle << endl;
        return port.status();
    }

    Result msg = (*port)->getMessageOrMakeThreadWait();
    if (!msg) {
        return msg.status();
    }

    return (*msg)->infoAddress.value();
}

