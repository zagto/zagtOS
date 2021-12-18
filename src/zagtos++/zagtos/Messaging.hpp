#pragma once

#include <vector>
#include <tuple>
#include <memory>
#include <zagtos/UUID.hpp>
#include <zagtos/HandleObject.hpp>
#include <functional>


namespace zagtos {
    class RemotePort : public HandleObject {
    public:
        RemotePort() {}
        RemotePort(RemotePort &) = delete;
        RemotePort(RemotePort &&other) : HandleObject(std::move(other)) {}

        void sendMessage(const UUID messageType,
                         zbon::EncodedData message) const;
    };

    struct MessageInfo {
        /* index into the ports array of a ReceiveMessage call */
        size_t portIndex;
        alignas(16) UUID type;
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
        static std::unique_ptr<MessageInfo> receiveMessage(std::vector<std::reference_wrapper<Port>> ports);
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
