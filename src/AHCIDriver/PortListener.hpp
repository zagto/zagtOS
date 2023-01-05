#pragma once

#include <zagtos/Messaging.hpp>

class PortListener {
public:
    zagtos::Port messagePort;

    PortListener() :
        messagePort(zagtos::DefaultEventQueue, reinterpret_cast<size_t>(this)) {}
    PortListener(const PortListener &other) = delete;
    virtual void handleMessage(const zagtos::Event &event) = 0;
};
