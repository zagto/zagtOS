#pragma once

#include <vector>
#include <tuple>
#include <memory>
#include <zagtos/UUID.hpp>
#include <zagtos/HandleObject.hpp>
#include <zagtos/KernelApi.h>
#include <zagtos/EventQueue.hpp>
#include <zagtos/MessageData.hpp>
#include <optional>

namespace zbon{
class Encoder;
class Decoder;

}

namespace zagtos {

class RemotePort : public HandleObject {
public:
    RemotePort() {}
    RemotePort(RemotePort &) = delete;
    RemotePort(RemotePort &&other) : HandleObject(std::move(other)) {}

    void sendMessage(const UUID messageType,
                     MessageData messageData) const;
};

class invalid_message : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Port : public HandleObject {
private:
    std::optional<EventQueue> privateEventQueue;

public:
    Port(EventQueue &eventQueue, size_t tag);
    Port();
    Port(Port &) = delete;
    Port(Port &&other) : HandleObject(std::move(other)) {}

    bool ZBONDecode(zbon::Decoder &) = delete;

    Event waitForMessage();
    template<typename T> T waitForMessage(UUID type) {
        T result;
        Event event = waitForMessage();
        if (event.messageType() != type) {
            throw invalid_message("waitForMessage: unexpected message type");
        }
        zbon::decode(event.messageData(), result);
        return result;
    }
};


extern "C" void exit(int);

const MessageData &RunMessageData();
const UUID RunMessageType();
void CheckRunMessageType(UUID type);
template<typename T> T decodeRunMessage(UUID type) {
    CheckRunMessageType(type);
    T result;
    zbon::decode(RunMessageData(), result);
    return result;
}

}
