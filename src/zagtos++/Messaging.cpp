#include <zagtos/syscall.h>
#include <zagtos/KernelApi.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/ZBON.hpp>
#include <cassert>
#include <sys/mman.h>

namespace zagtos {

MessageData::MessageData(MessageData &&other) {
    data = other.data;
    size = other.size;
    numHandles = other.numHandles;
    allocatedExternally = other.allocatedExternally;
    other.data = nullptr;
    other.size = 0;
    other.numHandles = 0;
    other.allocatedExternally = false;
}

MessageData::~MessageData() {
    if (data != nullptr && !allocatedExternally) {
        delete[] data;
    }
}

zbon::Size MessageData::ZBONSize() const {
    return {size - numHandles * zbon::HANDLE_SIZE, numHandles};
}

Port::Port(EventQueue &eventQueue, size_t tag) {
    size_t result = zagtos_syscall2(SYS_CREATE_PORT, eventQueue._handle, tag);
    _handle = static_cast<uint32_t>(result);
}

Port::Port() {
    /* TODO: this could become a single combined syscall if performance-relevant */
    privateEventQueue.emplace();
    size_t result = zagtos_syscall2(SYS_CREATE_PORT, privateEventQueue->_handle, 0);
    _handle = static_cast<uint32_t>(result);
}

Event Port::waitForMessage() {
    if (!privateEventQueue) {
        throw std::logic_error("Do not use Port::receiveMessage on ports that use a shared event "
                               "queue. Use EventQueue::waitForEvent");
    }
    return privateEventQueue->waitForEvent();
}

void RemotePort::sendMessage(UUID messageTypeID, MessageData messageData) const {
    zagtos_syscall5(SYS_SEND_MESSAGE,
                    _handle,
                    reinterpret_cast<size_t>(&messageTypeID),
                    reinterpret_cast<size_t>(messageData.data),
                    messageData.size,
                    messageData.numHandles);
}

extern "C" cApi::ZoProcessStartupInfo *__process_startup_info;

const MessageData &RunMessageData() {
    return static_cast<MessageData &>(__process_startup_info->runMessage.data);
}

const UUID RunMessageType() {
    return UUID(__process_startup_info->runMessage.type);
}

void CheckRunMessageType(UUID type) {
    if (RunMessageType() != type) {
        std::cout << "invalid run message type wanted: " << type << " got "
                  << RunMessageType() << std::endl;
        abort();
    }
}

}
