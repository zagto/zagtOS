#pragma once

#include <zagtos/Messaging.hpp>

namespace zagtos {

class EventListener {
public:
    zagtos::Port port;

    EventListener(zagtos::EventQueue& queue = zagtos::DefaultEventQueue) :
        port(queue, reinterpret_cast<size_t>(this)) {}
    EventListener(const EventListener &other) = delete;
    virtual void handleEvent(const zagtos::Event &event) = 0;
};

void DefaultEventLoop(zagtos::EventQueue& queue = zagtos::DefaultEventQueue);

}
