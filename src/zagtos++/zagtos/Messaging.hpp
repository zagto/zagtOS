#pragma once

#include <vector>
#include <uuid/uuid.h>
#include <zagtos/zbon.hpp>
#include <cstdint>

namespace zagtos {
    class ExternalBinary;

    struct Handle {
        uint32_t value;

        static zbon::Type ZBONType();
        zbon::Size ZBONSize() const;
        void ZBONEncode(zbon::Encoder &encoder) const;
    };

    class HandleObject {
    protected:
        static constexpr Handle INVALID_HANDLE{static_cast<uint32_t>(-1)};

        Handle _handle{INVALID_HANDLE};

    public:
        HandleObject();
        HandleObject(const Handle handle);

        Handle handle() const;
    };

    class RemotePort : public HandleObject { using HandleObject::HandleObject; };

    struct MessageInfo {
        uint32_t senderPort;
        uuid_t type;
        zbon::EncodedData data;
    };

    class Port : public HandleObject {
    private:
        bool valid;

    public:
        Port();
        Port(Port &) = delete;
        Port(Port &&ohter);
        ~Port();

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
