#pragma once

#include <vector>
#include <tuple>
#include <memory>
#include <zagtos/UUID.hpp>
#include <zagtos/ZBON.hpp>
#include <cstdint>
#include <functional>

namespace zagtos {
    class ExternalBinary;

    class HandleObject {
    protected:
        static constexpr uint32_t INVALID_HANDLE{static_cast<uint32_t>(-1)};

        uint32_t _handle{INVALID_HANDLE};

        HandleObject(const uint32_t handle);

    public:
        HandleObject();
        HandleObject(HandleObject &) = delete;
        ~HandleObject();
        HandleObject(HandleObject &&other);

        static constexpr zbon::Type ZBONType() {
            return zbon::Type::HANDLE;
        }
        zbon::Size ZBONSize() const;
        void ZBONEncode(zbon::Encoder &encoder) const;
        void ZBONDecode(zbon::Decoder &decoder);
    };

    class RemotePort : public HandleObject {
    public:
        RemotePort() {}
        RemotePort(RemotePort &) = delete;
        RemotePort(RemotePort &&other) : HandleObject(std::move(other)) {}

        void sendMessage(const UUID messageType,
                         zbon::EncodedData message) const;
    };

    class SharedMemory : public HandleObject {
    private:
        void *_map(int protection);

    public:
        static std::tuple<SharedMemory, std::vector<size_t>> DMA(size_t deviceMax, size_t length);
        static SharedMemory Physical(size_t physicalAddress, size_t length);
        static SharedMemory Standard(size_t length);

        SharedMemory() {}
        SharedMemory(SharedMemory &) = delete;
        SharedMemory(SharedMemory &&other) : HandleObject(std::move(other)) {}

        void operator=(SharedMemory && other);

        template<typename T> T *map(int protection) {
            return reinterpret_cast<T *>(_map(protection));
        }
    };
    void UnmapWhole(void *pointer);

    struct MessageInfo {
        /* index into the ports array of a ReceiveMessage call */
        size_t portIndex;
        UUID type;
        zbon::EncodedData data;

        static void *operator new(std::size_t);
        static void *operator new[](std::size_t);
        static void operator delete(void *object);
    };

    class Port : public HandleObject {
    public:
        Port();
        Port(Port &) = delete;
        Port(Port &&other) : HandleObject(std::move(other)) {}

        bool ZBONDecode(zbon::Decoder &) = delete;

        std::unique_ptr<MessageInfo> receiveMessage();
        template<typename T> void receiveMessage(UUID type, T &result) {
            while (true) {
                std::unique_ptr<MessageInfo> msgInfo = receiveMessage();
                if (type != msgInfo->type) {
                    std::cerr << "receiveMessage: invalid message type" << std::endl;
                } else if (!zbon::decode(msgInfo->data, result)) {
                    std::cerr << "receiveMessage: invalid data" << std::endl;
                } else {
                    return;
                }
            }
        }
        static std::pair<std::unique_ptr<MessageInfo>, size_t>
        receiveMessage(std::vector<std::reference_wrapper<Port>> ports);
    };


    extern "C" void exit(int);

    const MessageInfo &receiveRunMessageInfo();
    void receiveRunMessage(UUID type);
    template<typename T> T decodeRunMessage(UUID type) {
        const MessageInfo &msgInfo = receiveRunMessageInfo();
        if (type != msgInfo.type) {
            std::cerr << "invalid run message type" << std::endl;
            exit(1);
        }
        T result;
        zbon::decode(msgInfo.data, result);
        return result;
    }
}
