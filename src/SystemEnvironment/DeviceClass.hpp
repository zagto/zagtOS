#pragma once

#include "PortListener.hpp"
#include "DeviceClassSubscription.hpp"
#include <set>

class ClassDevice;

class DeviceClass : public PortListener {
private:
    std::vector<std::unique_ptr<DeviceClassSubscription>> subscriptions;
    std::vector<zagtos::RemotePort> instancePorts;
    std::vector<zagtos::MessageData> instanceData;

public:
    const zagtos::UUID id;

    DeviceClass(zagtos::UUID id) :
        id{id} {}
    void addInstance(zagtos::RemotePort remotePort, zagtos::MessageData data);
    void handleMessage(const zagtos::Event &event) final;
};

extern std::vector<std::shared_ptr<DeviceClass>> DeviceClassRegistry;
