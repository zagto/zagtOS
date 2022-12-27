#pragma once

#include <memory>
#include <zagtos/Messaging.hpp>

class DeviceClass;

struct ClassDevice {
    std::shared_ptr<DeviceClass> deviceClass;
    zagtos::RemotePort consumerPort;

    ClassDevice(std::shared_ptr<DeviceClass> deviceClass_, zagtos::RemotePort consumerPort);
    ~ClassDevice();
    ClassDevice(const ClassDevice &) = delete;
};
