#define _GNU_SOURCE 1
#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/Align.hpp>
#include <cassert>
#include <sys/mman.h>
#include <climits>

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

HandleObject::HandleObject(HandleObject &&other) {
    _handle = other._handle;
    other._handle = INVALID_HANDLE;
}

HandleObject::~HandleObject() {
    if (_handle != INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle);
    }
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

enum SharedType {
    STANDARD = 0,
    PHYSICAL = 1,
    DMA = 2,
};

std::tuple<SharedMemory, std::vector<size_t>> SharedMemory::DMA(size_t deviceMax, size_t length) {
    size_t numPages = zagtos::align(length, PAGE_SIZE, zagtos::AlignDirection::UP) / PAGE_SIZE;
    std::vector<size_t> deviceAddresses(numPages);
    SharedMemory shm;
    shm._handle = static_cast<uint32_t>(
                zagtos_syscall4(SYS_CREATE_SHARED_MEMORY,
                                SharedType::DMA,
                                static_cast<size_t>(deviceMax),
                                length,
                                reinterpret_cast<size_t>(deviceAddresses.data())));
    return {std::move(shm), std::move(deviceAddresses)};
}

SharedMemory SharedMemory::Physical(size_t physicalAddress, size_t length) {
    SharedMemory shm;
    shm._handle = static_cast<uint32_t>(
                zagtos_syscall4(SYS_CREATE_SHARED_MEMORY,
                                SharedType::PHYSICAL,
                                physicalAddress,
                                length,
                                0));
    return shm;
}

SharedMemory SharedMemory::Standard(size_t length) {
    SharedMemory shm;
    shm._handle = static_cast<uint32_t>(
                zagtos_syscall4(SYS_CREATE_SHARED_MEMORY,
                                SharedType::STANDARD,
                                0,
                                length,
                                0));
    return shm;
}

void SharedMemory::operator=(SharedMemory &&other) {
    if (_handle != INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle);
    }
    _handle = other._handle;
    other._handle = INVALID_HANDLE;
}

void *SharedMemory::_map(int protection) {
    assert(_handle != INVALID_HANDLE);
    return mmap(nullptr, 0, protection, MAP_SHARED|MAP_WHOLE, _handle, 0);
}

void UnmapWhole(void *pointer) {
    size_t ret = zagtos_syscall3(SYS_MUNMAP, 1, reinterpret_cast<size_t>(pointer), 0);
    assert(ret == 0);
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
