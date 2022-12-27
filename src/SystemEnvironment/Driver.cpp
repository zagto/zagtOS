#include "Driver.hpp"

std::vector<std::shared_ptr<Driver>> DriverRegistry;

bool Driver::matchesDevice(zagtos::UUID controller, uint64_t deviceID) const {
    for (auto &match: drivenDevices) {
        if (match.controller == controller &&match.id == (deviceID & match.idMask)) {
            return true;
        }
    }
    return false;
}

bool Driver::canProvideDeviceClass(zagtos::UUID deviceClass) const {
    for (auto &match: providesClassDevices) {
        if (match == deviceClass) {
            return true;
        }
    }
    return false;
}

bool Driver::canProvideController(zagtos::UUID controller) const {
    for (auto &match: providesControllers) {
        if (match == controller) {
            return true;
        }
    }
    return false;
}

const char *Driver::name() const {
    return binary.logName();
}
