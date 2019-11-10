#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <vector>
#include <uuid/uuid.h>
#include <zagtos/zbon.hpp>
#include <cstdint>

namespace zagtos {
    class ExternalBinary;

    class UUIDObject {
    protected:
        uuid_t _id;

    public:
        UUIDObject();
        UUIDObject(const uuid_t id);
        const uuid_t &id() const;
        static zbon::Type ZBONType();
        size_t ZBONSize() const;
        void ZBONEncode(zbon::Encoder &encoder) const;
    };

    class Protocol : public UUIDObject { using UUIDObject::UUIDObject; };
    class RemotePort : public UUIDObject { using UUIDObject::UUIDObject; };
    class MessageType : public UUIDObject { using UUIDObject::UUIDObject; };

    class Port {
    private:
        uint32_t id;
        bool valid;

    public:
        Port();
        Port(const std::vector<uint32_t> &acceptedTags);
        Port(Port &) = delete;
        Port(Port &&ohter);
        ~Port();
        uint32_t selfTag() const;
    };

    void sendMessage(const RemotePort &target,
                     uuid_t messageType,
                     zbon::EncodedData message);
}

#endif // MESSAGING_HPP
