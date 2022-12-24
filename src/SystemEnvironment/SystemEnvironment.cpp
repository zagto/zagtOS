#include <iostream>
#include <thread>
#include <tuple>
#include <vector>
#include <set>
#include <zagtos/ZBON.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/Firmware.hpp>
#include <zagtos/protocols/Pci.hpp>
#include <zagtos/protocols/Driver.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/EnvironmentSpawn.hpp>

EXTERNAL_BINARY(ACPIHAL)
EXTERNAL_BINARY(DeviceTreeHAL)
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

struct ClassDevice;

struct DeviceClass {
    UUID id;
    std::vector<RemotePort> consumers;
    std::set<ClassDevice *> instances;

    DeviceClass(UUID id) :
        id{id} {}
};

std::vector<std::shared_ptr<DeviceClass>> DeviceClassRegistry;

struct ClassDevice {
    std::shared_ptr<DeviceClass> deviceClass;
    RemotePort consumerPort;

    ClassDevice(std::shared_ptr<DeviceClass> deviceClass_, RemotePort consumerPort) :
        deviceClass{std::move(deviceClass_)},
        consumerPort{std::move(consumerPort)} {

        deviceClass->instances.insert(this);
    }
    ~ClassDevice() {
        deviceClass->instances.erase(this);
    }
    ClassDevice(const ClassDevice &) = delete;
};

class Device {
private:
    std::vector<std::unique_ptr<Device>> children;
    std::vector<std::unique_ptr<ClassDevice>> classDevices;
    std::shared_ptr<Driver> driver;
    Port port;

public:
    /* root device */
    Device(std::shared_ptr<Driver> halDriver) :
        driver{halDriver},
        port(DefaultEventQueue, reinterpret_cast<size_t>(this)) {

        auto runMessage = zbon::encode(port);
        environmentSpawn(driver->binary, Priority::BACKGROUND, driver::MSG_START_HAL, runMessage);
    }

    Device(UUID controllerType,
           uint64_t deviceID,
           const zagtos::MessageData &driverMessage) {
        for (auto &driver: DriverRegistry) {
            if (driver->matchesDevice(controllerType, deviceID)) {
                std::cout << "starting driver " << driver->name() << " for device" << std::endl;
                this->driver = driver;
                break;
            }
        }

        if (driver) {
            port = Port(DefaultEventQueue, reinterpret_cast<size_t>(this));
            auto runMessage = zbon::encodeObject(controllerType, port, driverMessage);
            environmentSpawn(driver->binary, Priority::BACKGROUND, driver::MSG_START, runMessage);
        }
    }

    void addChild(std::unique_ptr<Device> child) {
        children.push_back(std::move(child));
    }

    void foundChildDevice(UUID controllerType,
                          uint64_t deviceID,
                          const zagtos::MessageData &driverMessage) {
        if (!driver->canProvideController(controllerType)) {
            std::cout << driver->name() << " found a device on a controller it is not allowed to provide" << std::endl;
            return;
        }

        auto childDevice = std::make_unique<Device>(
                    controllerType,
                    deviceID,
                    driverMessage);
        children.push_back(std::move(childDevice));
    }

    void foundClassDevice(UUID deviceClassID, RemotePort consumerPort) {
        if (!driver->canProvideDeviceClass(deviceClassID)) {
            std::cout << driver->name() << " found a class device by driver that is not "
                      << "allowed provide it" << std::endl;
            return;
        }

        for (std::shared_ptr<DeviceClass> &deviceClass: DeviceClassRegistry) {
            if (deviceClass->id == deviceClassID) {
                auto classDevice = std::make_unique<ClassDevice>(deviceClass,
                                                                 std::move(consumerPort));
                //for (RemotePort &consumerPort : deviceClass->consumers) {
                    // TODO: send message
                //}
                classDevices.push_back(std::move(classDevice));
                return;
            }
        }

        std::cout << "Unsupported device class: " << deviceClassID << std::endl;
    }

    const char *name() {
        return driver->name();
    }
};

std::unique_ptr<Device> DeviceTree;

void registerEmbeddedDrivers() {
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        ACPIHAL,
        {},
        {},
        {driver::CONTROLLER_TYPE_ROOT}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        DeviceTreeHAL,
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
        {driver::DEVICE_CLASS_BLOCK_STORAGE},
        {}}));

    DeviceClassRegistry.push_back(std::make_shared<DeviceClass>(driver::DEVICE_CLASS_BLOCK_STORAGE));
}

int main() {
    CheckRunMessageType(MSG_BE_INIT);

    std::cout << "Setup..." << std::endl;

    registerEmbeddedDrivers();

    std::cout << "Starting HAL..." << std::endl;

    FirmwareInfo firmwareInfo = GetFirmwareInfo();
    if (firmwareInfo.type == cApi::ZAGTOS_FIRMWARE_TYPE_ACPI) {
        std::cout << "Detected firmware type: ACPI" << std::endl;
        /* create root device (ACPIHAL) */
        DeviceTree = std::make_unique<Device>(DriverRegistry[0]);
    } else if (firmwareInfo.type == cApi::ZAGTOS_FIRMWARE_TYPE_DTB) {
        std::cout << "Detected firmware type: DTB" << std::endl;
        /* create root device (DeviceTreeHAL) */
        DeviceTree = std::make_unique<Device>(DriverRegistry[1]);
    } else {
        throw std::runtime_error("Unknown firmware type");
    }

    while (true) {
        Event event = DefaultEventQueue.waitForEvent();
        Device *sender = reinterpret_cast<Device *>(event.tag());
        std::cout << "Message from " << sender->name() << std::endl;

        if (event.messageType() == driver::MSG_START_RESULT) {
            bool result;
            try {
                zbon::decode(event.messageData(), result);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_START_RESULT message." << std::endl;
                continue;
            }

            if (result) {
                std::cout << sender->name() << " startup complete. TODO: what's next?" << std::endl;
            } else {
                std::cout << sender->name() << " startup failed." << std::endl;
            }
        } else if (event.messageType() == driver::MSG_FOUND_DEVICE) {
            std::tuple<UUID, uint64_t, zbon::EncodedData> msg;
            try {
                zbon::decode(event.messageData(), msg);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_FOUND_DEVICE message." << std::endl;
                continue;
            }
            sender->foundChildDevice(std::get<0>(msg),
                                     std::get<1>(msg),
                                     std::move(std::get<2>(msg)));
        } else if (event.messageType() == driver::MSG_FOUND_CLASS_DEVICE) {
            std::tuple<UUID, RemotePort> msg;
            try {
                zbon::decode(event.messageData(), msg);
            } catch(zbon::DecoderException &e) {
                std::cout << "Received malformed MSG_FOUND_CLASS_DEVICE message." << std::endl;
                continue;
            }
            auto [deviceClass, consumerPort] = std::move(msg);
            sender->foundClassDevice(deviceClass, std::move(consumerPort));
        } else {
            std::cout << "Got message of unknown type from HAL" << std::endl;
        }
    }
}
