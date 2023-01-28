#pragma once

#include "Device.hpp"
#include <vector>
#include <optional>
#include <memory>

class DeviceList {
private:
    std::vector<std::unique_ptr<Device>> devices;

    DeviceList() {}
    DeviceList(DeviceList &other) = delete;
    std::optional<size_t> findIndexByID(uint64_t deviceID) const;

public:
    static DeviceList instance;

    void add(std::unique_ptr<Device> device);
    void remove(Device *device);
    void removeByID(uint64_t deviceID);
};
