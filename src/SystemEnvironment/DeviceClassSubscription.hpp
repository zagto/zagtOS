#pragma once

#include <zagtos/EventListener.hpp>

struct DeviceClassSubscription : public zagtos::EventListener {
    zagtos::RemotePort remotePort;

    DeviceClassSubscription(zagtos::RemotePort &&remotePort) :
        remotePort{std::move(remotePort)} {}
    void handleEvent(const zagtos::Event &event) final;
};
