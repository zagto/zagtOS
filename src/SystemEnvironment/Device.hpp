#pragma once

#include "Driver.hpp"
#include <zagtos/EventListener.hpp>

class Driver;

class Device : public zagtos::EventListener {
private:
    std::vector<std::unique_ptr<Device>> children;
    std::shared_ptr<Driver> driver;

public:
    Device(zagtos::UUID controllerType,
           uint64_t deviceID,
           const zagtos::MessageData &driverMessage);
    /* root device only */
    Device(std::shared_ptr<Driver> halDriver);
    void addChild(std::unique_ptr<Device> child);
    void foundChildDevice(zagtos::UUID controllerType,
                          uint64_t deviceID,
                          const zagtos::MessageData &driverMessage);
    void foundClassDevice(zagtos::UUID deviceClassID,
                          zagtos::RemotePort consumerPort,
                          zagtos::MessageData consumerData);
    const char *name();
    void handleEvent(const zagtos::Event &event) final;
};

extern std::unique_ptr<Device> DeviceTree;
