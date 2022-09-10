#include <zagtos/syscall.h>
#include <zagtos/KernelApi.h>
#include <zagtos/Messaging.hpp>
#include <cassert>
#include <sys/mman.h>

namespace zagtos {

void *MessageInfo::operator new(std::size_t) {
    throw std::logic_error("MessageInfo objects should only be allocated by the kernel");
}

void *MessageInfo::operator new[](std::size_t) {
    throw std::logic_error("MessageInfo objects should only be allocated by the kernel");
}

void MessageInfo::operator delete(void *object) {
    MessageInfo *msgInfo = reinterpret_cast<MessageInfo *>(object);
    size_t munmapSize = reinterpret_cast<size_t>(msgInfo->data.data())
            + msgInfo->data.size()
            - reinterpret_cast<size_t>(msgInfo);
    int result = munmap(object, munmapSize);
    assert(result == 0);
}


Port::Port() {
    _handle = {static_cast<uint32_t>(zagtos_syscall0(SYS_CREATE_PORT))};
}

std::unique_ptr<MessageInfo> Port::receiveMessage() {
    /* will be overwritten by result index, so don't use the member variable directly */
    size_t tempHandle = _handle;
    size_t result = zagtos_syscall2(SYS_RECEIVE_MESSAGE, reinterpret_cast<size_t>(&tempHandle), 1);
    assert(reinterpret_cast<MessageInfo *>(result)->portIndex == 0);
    return std::unique_ptr<MessageInfo>(reinterpret_cast<MessageInfo *>(result));
}

std::unique_ptr<MessageInfo>
Port::receiveMessage(std::vector<std::reference_wrapper<Port>> ports) {
    std::vector<uint32_t> handles(ports.size());
    for (size_t index = 0; index < ports.size(); index++) {
        handles[index] = ports[index].get()._handle;
    }
    size_t result = zagtos_syscall2(SYS_RECEIVE_MESSAGE, reinterpret_cast<size_t>(handles.data()), handles.size());
    return std::unique_ptr<MessageInfo>(reinterpret_cast<MessageInfo *>(result));
}

void RemotePort::sendMessage(UUID messageTypeID, zbon::EncodedData message) const {
    zagtos_syscall5(SYS_SEND_MESSAGE,
                    _handle,
                    reinterpret_cast<size_t>(&messageTypeID),
                    reinterpret_cast<size_t>(message.data()),
                    message.size(),
                    message.numHandles());
}

extern "C" ZoProcessStartupInfo *__process_startup_info;


const MessageInfo &receiveRunMessageInfo() {
    return *reinterpret_cast<MessageInfo *>(&__process_startup_info->runMessage);
}

void receiveRunMessage(UUID type) {
    auto messageInfo = reinterpret_cast<MessageInfo *>(&__process_startup_info->runMessage);
    if (type != messageInfo->type) {
        std::cout << "invalid run message type wanted: " << type << " got "
                  << messageInfo->type << std::endl;
        exit(1);
    }
}

}
