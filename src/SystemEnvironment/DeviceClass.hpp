#pragma once

#include "DeviceClassSubscription.hpp"
#include <zagtos/EventListener.hpp>
#include <set>

class ClassDevice;

class DeviceClass : public zagtos::EventListener {
private:
    std::vector<std::unique_ptr<DeviceClassSubscription>> subscriptions;
    std::vector<zagtos::RemotePort> instancePorts;
    std::vector<zagtos::MessageData> instanceData;
    std::vector<uint64_t> instanceIDs;
    uint64_t nextInstanceID = 0;

public:
    const zagtos::UUID id;

    DeviceClass(zagtos::UUID id) :
        id{id} {}
    void addInstance(zagtos::RemotePort remotePort, zagtos::MessageData data);
    void handleEvent(const zagtos::Event &event) final;
};

extern std::vector<std::shared_ptr<DeviceClass>> DeviceClassRegistry;
