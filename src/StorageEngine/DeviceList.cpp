#include "DeviceList.hpp"

DeviceList DeviceList::instance;

std::optional<size_t> DeviceList::findIndexByID(uint64_t deviceID) const {
    for (size_t index = 0; index < devices.size(); index++) {
        if (devices[index]->id == deviceID) {
            return index;
        }
    }
    return {};
}

void DeviceList::add(std::unique_ptr<Device> device) {
    if (findIndexByID(device->id)) {
        throw std::logic_error("Adding device ID");
    }
    devices.push_back(std::move(device));
}

void DeviceList::remove(Device *device) {
    for (size_t index = 0; index < devices.size(); index++) {
        if (devices[index].get() == device) {
            devices.erase(devices.begin() + index);
            return;
        }
    }
    throw std::logic_error("Removing Device object that is not in list");
}

void DeviceList::removeByID(uint64_t deviceID) {
    std::optional<size_t> index = *findIndexByID(deviceID);
    if (!index) {
        throw std::logic_error("Removing non-existing device ID");
    }
    devices.erase(devices.begin() + *index);
}
