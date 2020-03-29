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
    static constexpr bool operator==(const Handle a, const Handle b) {
        return a.value == b.value;
    }
    static constexpr bool operator!=(const Handle a, const Handle b) {
        return a.value != b.value;
    }

    class HandleObject {
    protected:
        static constexpr Handle INVALID_HANDLE{static_cast<uint32_t>(-1)};

        Handle _handle{INVALID_HANDLE};

    public:
        HandleObject();
        HandleObject(const Handle handle);

        Handle handle() const;

        static constexpr zbon::Type ZBONType() {
            return zbon::Type::HANDLE;
        }
        bool ZBONDecode(zbon::Decoder &decoder);
    };

    class RemotePort : public HandleObject {
    public:
        RemotePort() {}
        RemotePort(RemotePort &) = delete;
        RemotePort(RemotePort &&ohter);
        ~RemotePort();

    };

    struct MessageInfo {
        uuid_t type;
        zbon::EncodedData data;
    };

    class Port : public HandleObject {
    public:
        Port();
        Port(Port &) = delete;
        Port(Port &&ohter);
        ~Port();

        bool ZBONDecode(zbon::Decoder &) = delete;

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


    extern "C" void exit(int);

    const MessageInfo &receiveRunMessage();
    template<typename T> T receiveRunMessage(const uuid_t type) {
        const MessageInfo &msgInfo = receiveRunMessage();
        if (uuid_compare(type, msgInfo.type) != 0) {
            std::cerr << "invalid run message type" << std::endl;
            exit(1);
        }
        T result;
        if (!zbon::decode(msgInfo.data, result)) {
            std::cerr << "could not decode run message" << std::endl;
            exit(1);
        }
        return result;
    }
    void receiveRunMessage(const uuid_t type);
}
