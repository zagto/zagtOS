#pragma once

#include <vector>
#include <memory>
#include <uuid/uuid.h>
#include <zagtos/ZBON.hpp>
#include <cstdint>

namespace zagtos {
    class ExternalBinary;

    class HandleObject {
    protected:
        static constexpr uint32_t INVALID_HANDLE{static_cast<uint32_t>(-1)};

        uint32_t _handle{INVALID_HANDLE};

        HandleObject(const uint32_t handle);

    public:
        HandleObject();
        ~HandleObject();

        static constexpr zbon::Type ZBONType() {
            return zbon::Type::HANDLE;
        }
        zbon::Size ZBONSize() const;
        void ZBONEncode(zbon::Encoder &encoder) const;
        bool ZBONDecode(zbon::Decoder &decoder);
    };

    class RemotePort : public HandleObject {
    public:
        RemotePort() {}
        RemotePort(RemotePort &) = delete;
        RemotePort(RemotePort &&ohter);

        void sendMessage(const uuid_t messageType,
                         zbon::EncodedData message) const;
    };

    class SharedMemory : public HandleObject {
    public:
        SharedMemory() {}
        SharedMemory(int flags, size_t offset, size_t length);
        SharedMemory(SharedMemory &) = delete;
        SharedMemory(SharedMemory &&ohter);
    };

    struct MessageInfo {
        uuid_t type;
        zbon::EncodedData data;

        static void *operator new(std::size_t);
        static void *operator new[](std::size_t);
        static void operator delete(void *object);
    };

    class Port : public HandleObject {
    public:
        Port();
        Port(Port &) = delete;
        Port(Port &&ohter);

        bool ZBONDecode(zbon::Decoder &) = delete;

        std::unique_ptr<MessageInfo> receiveMessage();
        template<typename T> void receiveMessage(const uuid_t type, T &result) {
            while (true) {
                std::unique_ptr<MessageInfo> msgInfo = receiveMessage();
                if (uuid_compare(type, msgInfo->type) != 0) {
                    std::cerr << "receiveMessage: invalid message type" << std::endl;
                } else if (!zbon::decode(msgInfo->data, result)) {
                    std::cerr << "receiveMessage: invalid data" << std::endl;
                } else {
                    result = 1;
                    return;
                }
            }
        }
    };



    extern "C" void exit(int);

    const MessageInfo &receiveRunMessageInfo();
    void receiveRunMessage(const uuid_t type);
    template<typename T> T decodeRunMessage(const uuid_t type) {
        const MessageInfo &msgInfo = receiveRunMessageInfo();
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
}
