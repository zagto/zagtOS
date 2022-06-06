#include <iostream>
#include <thread>
#include <tuple>
#include <vector>
#include <zagtos/ZBON.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Pci.hpp>
#include <zagtos/protocols/Driver.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/EnvironmentSpawn.hpp>

EXTERNAL_BINARY(ACPIHAL)
EXTERNAL_BINARY(PCIController)
EXTERNAL_BINARY(AHCIDriver)
EXTERNAL_BINARY(PS2Controller)

using namespace zagtos;

static constexpr UUID MSG_BE_INIT(
        0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
        0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55);

struct DeviceMatch {
    UUID controller;
    uint64_t id;
    uint64_t idMask;

    static constexpr uint64_t EXACT_MASK = -1;
};

class Driver;
struct RunningDriver {
    std::shared_ptr<Driver> driver;
    std::unique_ptr<Port> port;
};
std::vector<RunningDriver> RunningDrivers;
std::vector<std::reference_wrapper<Port>> AllPorts;

struct Driver {
    const ExternalBinary &binary;
    std::vector<DeviceMatch> drivenDevices;
    std::vector<UUID> providesClassDevices;
    std::vector<UUID> providesControllers;

    bool matchesDevice(UUID controller, uint64_t deviceID) const {
        for (auto &match: drivenDevices) {
            if (match.controller == controller &&match.id == (deviceID & match.idMask)) {
                return true;
            }
        }
        return false;
    }

    bool canProvideDeviceClass(UUID deviceClass) const {
        for (auto &match: providesClassDevices) {
            if (match == deviceClass) {
                return true;
            }
        }
        return false;
    }

    bool canProvideController(UUID controller) const {
        for (auto &match: providesControllers) {
            if (match == controller) {
                return true;
            }
        }
        return false;
    }

    const char *name() const {
        return binary.logName();
    }
};


std::vector<std::shared_ptr<Driver>> DriverRegistry;

void StartDriver(std::shared_ptr<Driver> driver,
                 UUID onController,
                 const zbon::EncodedData &message) {
    auto port = std::make_unique<Port>();
    auto runMessage = zbon::encodeObject(onController, *port, message);
    environmentSpawn(driver->binary, Priority::BACKGROUND, driver::MSG_START, runMessage);

    AllPorts.push_back(*port);
    RunningDrivers.push_back({std::move(driver), std::move(port)});
}

void registerEmbeddedDrivers() {
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        ACPIHAL,
        {},
        {},
        {driver::CONTROLLER_TYPE_ROOT}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        PCIController,
        {{driver::CONTROLLER_TYPE_ROOT, zagtos::driver::RootDevice::PCI_CONTROLLER, DeviceMatch::EXACT_MASK}},
        {},
        {driver::CONTROLLER_TYPE_PCI}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        PS2Controller,
        {{driver::CONTROLLER_TYPE_ROOT, zagtos::driver::RootDevice::PS2_CONTROLLER, DeviceMatch::EXACT_MASK}},
        {},
        {driver::CONTROLLER_TYPE_PS2}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        AHCIDriver,
        {{driver::CONTROLLER_TYPE_PCI, 0x0106'0000'0000'0000, 0xffff'0000'0000'0000}},
        {},
        {}}));
}

int main() {
    receiveRunMessage(MSG_BE_INIT);

    std::cout << "Setup..." << std::endl;

    registerEmbeddedDrivers();

    std::cout << "Starting HAL..." << std::endl;

    StartDriver(DriverRegistry[0], driver::CONTROLLER_TYPE_ROOT, zbon::encode(0));

    while (true) {
        std::unique_ptr<MessageInfo> msgInfo = Port::receiveMessage(
                    std::vector<std::reference_wrapper<Port>>(AllPorts.begin(), AllPorts.end()));
        RunningDriver &sender = RunningDrivers[msgInfo->portIndex];
        std::cout << "Message from " << sender.driver->name() << " (" << msgInfo->portIndex << ")" << std::endl;

        if (msgInfo->type == driver::MSG_START_RESULT) {
            bool result;
            try {
                zbon::decode(msgInfo->data, result);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_START_RESULT message." << std::endl;
                continue;
            }

            if (result) {
                std::cout << sender.driver->name() << " startup complete. TODO: what's next?" << std::endl;
            } else {
                std::cout << sender.driver->name() << " startup failed." << std::endl;
            }
        } else if (msgInfo->type == driver::MSG_FOUND_DEVICE) {
            std::tuple<UUID, uint64_t, zbon::EncodedData> msg;
            try {
                zbon::decode(msgInfo->data, msg);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_FOUND_DEVICE message." << std::endl;
                continue;
            }

            UUID controllerType = std::get<0>(msg);
            uint64_t deviceID = std::get<1>(msg);
            auto driverMessage = std::move(std::get<2>(msg));

            if (!sender.driver->canProvideController(controllerType)) {
                std::cout << sender.driver->name() << " found a device on a controller it is not allowed to provide" << std::endl;
                continue;
            }

            for (auto &driver: DriverRegistry) {
                if (driver->matchesDevice(controllerType, deviceID)) {
                    Port port;
                    std::cout << "starting driver " << driver->name() << " for device found by " << sender.driver->name() << std::endl;
                    StartDriver(driver, controllerType, driverMessage);
                    break;
                }
            }
        } else {
            std::cout << "Got message of unknown type from HAL" << std::endl;
        }
    }
}
