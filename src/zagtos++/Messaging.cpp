#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>

using namespace zagtos;

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


Port::Port() :
        valid{true} {
    _handle = {static_cast<uint32_t>(zagtos_syscall0(SYS_CREATE_PORT))};
}

Port::Port(Port &&other):
        valid{other.valid} {
    assert(_handle.value == other._handle.value);
    other._handle = {0};
    other.valid = false;
}

Port::~Port() {
    if (valid) {
        zagtos_syscall1(SYS_DESTROY_PORT, _handle.value);
    }
}

MessageInfo Port::receiveMessage() {
    MessageInfo result;
    zagtos_syscall2(SYS_RECEIVE_MESSAGE, _handle.value, reinterpret_cast<size_t>(&result));
    return result;
}


void zagtos::sendMessage(const RemotePort &target, uuid_t messageTypeID, zbon::EncodedData message) {
    zagtos_syscall5(SYS_SEND_MESSAGE,
                    target.handle().value,
                    reinterpret_cast<size_t>(&messageTypeID),
                    reinterpret_cast<size_t>(message.data()),
                    message.size(),
                    message.numHandles());
}

extern RunMessageInfo *__run_message;

const RunMessageInfo &zagtos::receiveRunMessage() {
    return *__run_message;
}

void zagtos::receiveRunMessage(const uuid_t type) {
    if (uuid_compare(type, __run_message->type) != 0) {
        std::cout << "invalid run message type" << std::endl;
        exit(1);
    }
}
