#pragma once

#include "PortListener.hpp"

struct DeviceClassSubscription : public PortListener {
    zagtos::RemotePort remotePort;

    DeviceClassSubscription(zagtos::RemotePort &&remotePort) :
        remotePort{std::move(remotePort)} {}
    void handleMessage(const zagtos::Event &event) final;
};
