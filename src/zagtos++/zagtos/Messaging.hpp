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

    struct MessageInfo {
        uint32_t senderPort;
        uuid_t type;
        zbon::EncodedData data;
    };

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
        MessageInfo receiveMessage();
        template<typename T> void receiveMessage(const uuid_t type, T &result) {
            while (true) {
                MessageInfo msgInfo = receiveMessage();
                if (uuid_compare(type, msgInfo.type) == 0 && zbon::decode(msgInfo.data, result)) {
                    return;
                }
            }
        }
    };

    void sendMessage(const RemotePort &target,
                     uuid_t messageType,
                     zbon::EncodedData message);


    struct RunMessageInfo {
        uuid_t type;
        zbon::EncodedData encodedData;
    };

    extern "C" void exit(int);

    const RunMessageInfo &receiveRunMessage();
    void receiveRunMessage(const uuid_t type);
    template<typename T> void receiveRunMessage(const uuid_t type, T &result) {
        const RunMessageInfo &msgInfo = receiveRunMessage();
        if (uuid_compare(type, msgInfo.type) != 0) {
            std::cerr << "invalid run message type" << std::endl;
            exit(1);
        }
        if (!zbon::decode(msgInfo, result)) {
            std::cerr << "could not decode run message" << std::endl;
            exit(1);
        }
        return result;
    }
}

#endif // MESSAGING_HPP
