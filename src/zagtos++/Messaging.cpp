#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>
#include <cassert>
#include <sys/mman.h>
#include <limits.h>

namespace zagtos {

HandleObject::HandleObject() {}

zbon::Size HandleObject::ZBONSize() const {
    return {1, 1};
}

void HandleObject::ZBONEncode(zbon::Encoder &encoder) const {
    encoder.encodeHandle(_handle);
}

void HandleObject::ZBONDecode(zbon::Decoder &decoder) {
    assert(_handle == INVALID_HANDLE);

    decoder.decodeHandle(_handle);
}


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

Port::Port(Port &&other) {
    _handle = other._handle;
    other._handle = INVALID_HANDLE;
}

RemotePort::RemotePort(RemotePort &&other) {
    _handle = other._handle;
    other._handle = INVALID_HANDLE;
}

SharedMemory::SharedMemory(size_t physicalAddress, size_t length) {
    _handle = {static_cast<uint32_t>(zagtos_syscall3(SYS_CREATE_SHARED_MEMORY, 1, physicalAddress, length))};
}

SharedMemory::SharedMemory(size_t length) {
    _handle = {static_cast<uint32_t>(zagtos_syscall3(SYS_CREATE_SHARED_MEMORY, 0, 0, length))};
}

SharedMemory::SharedMemory(SharedMemory &&other) {
    _handle = other._handle;
    other._handle = INVALID_HANDLE;
}

HandleObject::~HandleObject() {
    if (_handle != INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle);
    }
}

void *SharedMemory::map(int protection) {
    assert(_handle != INVALID_HANDLE);
    return mmap(nullptr, 0, protection, MAP_SHARED|MAP_WHOLE, _handle, 0);
}

std::unique_ptr<MessageInfo> Port::receiveMessage() {
    size_t result = zagtos_syscall2(SYS_RECEIVE_MESSAGE, _handle, reinterpret_cast<size_t>(&result));
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

extern "C" MessageInfo *__run_message;

const MessageInfo &receiveRunMessageInfo() {
    return *__run_message;
}

void receiveRunMessage(UUID type) {
    if (type != __run_message->type) {
        std::cout << "invalid run message type" << std::endl;
        exit(1);
    }
}

}
