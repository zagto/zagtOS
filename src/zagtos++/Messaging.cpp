#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>

using namespace zagtos;

UUIDObject::UUIDObject() {
    uuid_clear(_id);
}

UUIDObject::UUIDObject(const uuid_t id) {
    uuid_copy(_id, id);
}

const uuid_t &UUIDObject::id() const {
    return _id;
}

zbon::Type UUIDObject::ZBONType() {
    return zbon::Type::ARRAY;
}

size_t UUIDObject::ZBONSize() const {
    return zbon::sizeFor(_id);
}

void UUIDObject::ZBONEncode(zbon::Encoder &encoder) const {
    encoder.encodeValue(_id);
}

Port::Port():
        UUIDObject(),
        valid{true} {
    zagtos_syscall3(SYS_CREATE_PORT, 0, 0, reinterpret_cast<size_t>(&_id));
    valid = true;
}

Port::Port(const std::vector<uuid_t> &protocols):
        valid{true} {
    zagtos_syscall3(SYS_CREATE_PORT,
                    reinterpret_cast<size_t>(protocols.data()),
                    protocols.size(),
                    reinterpret_cast<size_t>(&_id));
}

Port::Port(Port &&other):
        UUIDObject(other._id),
        valid{other.valid} {
    uuid_clear(other._id);
}

Port::~Port() {
    if (valid) {
        zagtos_syscall1(SYS_DESTROY_PORT, reinterpret_cast<size_t>(this));
    }
}

Protocol Port::selfProtocol() const {
    return Protocol(_id);
}

void zagtos::sendMessage(const RemotePort &target, uuid_t messageTypeID, zbon::EncodedData message) {
    zagtos_syscall4(SYS_SEND_MESSAGE,
                    reinterpret_cast<size_t>(&target.id()),
                    reinterpret_cast<size_t>(&messageTypeID),
                    reinterpret_cast<size_t>(message.data()),
                    message.size());
}
