#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>
#include <cassert>
#include <sys/mman.h>
#include <limits.h>

namespace zagtos {

HandleObject::HandleObject() {}

HandleObject::HandleObject(const Handle handle) {
    _handle = handle;
}

Handle HandleObject::handle() const {
    return _handle;
}

zbon::Type Handle::ZBONType() {
    return zbon::Type::HANDLE;
}

zbon::Size Handle::ZBONSize() const {
    return {0, 1};
}

void Handle::ZBONEncode(zbon::Encoder &encoder) const {
    encoder.encodeHandle(value);
}

bool HandleObject::ZBONDecode(zbon::Decoder &decoder) {
    assert(_handle == INVALID_HANDLE);

    bool success = decoder.decodeHandle(_handle.value);
    if (!success) {
        _handle = INVALID_HANDLE;
    }
    return success;
}


void *MessageInfo::operator new(std::size_t) {
    throw std::logic_error("MessageInfo objects should only be allocated by the kernel");
}

void *MessageInfo::operator new[](std::size_t) {
    throw std::logic_error("MessageInfo objects should only be allocated by the kernel");
}

void MessageInfo::operator delete(void *object) {
    std::cout << "unmapping message at " << object << std::endl;
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

Port::Port(Port &&other) {
    assert(_handle == other._handle);
    other._handle = INVALID_HANDLE;
}

Port::~Port() {
    if (_handle != INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle.value);
    }
}

RemotePort::RemotePort(RemotePort &&other) {
    assert(_handle == other._handle);
    other._handle = INVALID_HANDLE;
}

RemotePort::~RemotePort() {
    if (_handle != INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle.value);
    }
}


std::unique_ptr<MessageInfo> Port::receiveMessage() {
    size_t result = zagtos_syscall2(SYS_RECEIVE_MESSAGE, _handle.value, reinterpret_cast<size_t>(&result));
    return std::unique_ptr<MessageInfo>(reinterpret_cast<MessageInfo *>(result));
}


void sendMessage(const RemotePort &target, const uuid_t messageTypeID, zbon::EncodedData message) {
    zagtos_syscall5(SYS_SEND_MESSAGE,
                    target.handle().value,
                    reinterpret_cast<size_t>(messageTypeID),
                    reinterpret_cast<size_t>(message.data()),
                    message.size(),
                    message.numHandles());
}

extern "C" MessageInfo *__run_message;

const MessageInfo &receiveRunMessageInfo() {
    return *__run_message;
}

void receiveRunMessage(const uuid_t type) {
    if (uuid_compare(type, __run_message->type) != 0) {
        std::cout << "invalid run message type" << std::endl;
        exit(1);
    }
}

}
