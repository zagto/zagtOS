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

Port::Port() :
        valid{true} {
    id = static_cast<uint32_t>(zagtos_syscall2(SYS_CREATE_PORT, 0, 0));
}

Port::Port(const std::vector<uint32_t> &acceptedTags):
        valid{true} {
    id = static_cast<uint32_t>(zagtos_syscall2(SYS_CREATE_PORT,
                               reinterpret_cast<size_t>(acceptedTags.data()),
                               acceptedTags.size()));
}

Port::Port(Port &&other):
        id{other.id},
        valid{other.valid} {
    other.id = 0;
    other.valid = 0;
}

Port::~Port() {
    if (valid) {
        zagtos_syscall1(SYS_DESTROY_PORT, id);
    }
}

uint32_t Port::selfTag() const {
    return id;
}

MessageInfo Port::receiveMessage() {
    MessageInfo result;
    zagtos_syscall1(SYS_RECEIVE_MESSAGE, reinterpret_cast<size_t>(&result));
    return result;
}

void zagtos::sendMessage(const RemotePort &target, uuid_t messageTypeID, zbon::EncodedData message) {
    zagtos_syscall4(SYS_SEND_MESSAGE,
                    reinterpret_cast<size_t>(&target.id()),
                    reinterpret_cast<size_t>(&messageTypeID),
                    reinterpret_cast<size_t>(message.data()),
                    message.size());
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
