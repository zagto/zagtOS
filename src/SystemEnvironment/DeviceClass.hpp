#pragma once

#include "PortListener.hpp"
#include <set>

class ClassDevice;

struct DeviceClass : public PortListener {
    const zagtos::UUID id;
    std::vector<zagtos::RemotePort> consumers;
    std::set<ClassDevice *> instances;
    zagtos::Port subscribePort;

    DeviceClass(zagtos::UUID id) :
        id{id} {}
    void handleMessage(const zagtos::Event &event) override;
};

extern std::vector<std::shared_ptr<DeviceClass>> DeviceClassRegistry;
