#pragma once

#include <zagtos/Messaging.hpp>

class PortListener {
protected:
    zagtos::Port port;

public:
    virtual void handleMessage(const zagtos::Event &event) = 0;
};
