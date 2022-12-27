#pragma once

#include <cstdint>
#include <zagtos/UUID.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <memory>

struct DeviceMatch {
    zagtos::UUID controller;
    uint64_t id;
    uint64_t idMask;

    static constexpr uint64_t EXACT_MASK = -1;
};

class Driver {
private:
    std::vector<DeviceMatch> drivenDevices;
    std::vector<zagtos::UUID> providesClassDevices;
    std::vector<zagtos::UUID> providesControllers;

public:
    const zagtos::ExternalBinary &binary;
    bool matchesDevice(zagtos::UUID controller, uint64_t deviceID) const;
    bool canProvideDeviceClass(zagtos::UUID deviceClass) const;
    bool canProvideController(zagtos::UUID controller) const;
    const char *name() const;

    Driver(const zagtos::ExternalBinary &binary,
           std::vector<DeviceMatch> drivenDevices,
           std::vector<zagtos::UUID> providesClassDevices,
           std::vector<zagtos::UUID> providesControllers) :
        drivenDevices{std::move(drivenDevices)},
        providesClassDevices{std::move(providesClassDevices)},
        providesControllers{std::move(providesControllers)},
        binary{binary} {}
};

extern std::vector<std::shared_ptr<Driver>> DriverRegistry;
