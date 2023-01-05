#include "Device.hpp"
#include "Driver.hpp"
#include "DeviceClass.hpp"
#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/protocols/Driver.hpp>

std::unique_ptr<Device> DeviceTree;

Device::Device(zagtos::UUID controllerType,
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
        port = zagtos::Port(zagtos::DefaultEventQueue, reinterpret_cast<size_t>(this));
        auto runMessage = zbon::encodeObject(controllerType, port, driverMessage);
        environmentSpawn(driver->binary,
                         zagtos::Priority::BACKGROUND,
                         zagtos::driver::MSG_START, runMessage);
    }
}

/* root device only */
Device::Device(std::shared_ptr<Driver> halDriver) :
    driver{halDriver} {

    auto runMessage = zbon::encode(port);
    environmentSpawn(driver->binary,
                     zagtos::Priority::BACKGROUND,
                     zagtos::driver::MSG_START_HAL,
                     runMessage);
}

void Device::addChild(std::unique_ptr<Device> child) {
    children.push_back(std::move(child));
}

void Device::foundChildDevice(zagtos::UUID controllerType,
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

void Device::foundClassDevice(zagtos::UUID deviceClassID,
                              zagtos::RemotePort consumerPort,
                              zagtos::MessageData consumerData) {
    if (!driver->canProvideDeviceClass(deviceClassID)) {
        std::cout << driver->name() << " found a class device by driver that is not "
                  << "allowed provide it" << std::endl;
        return;
    }

    for (std::shared_ptr<DeviceClass> &deviceClass: DeviceClassRegistry) {
        if (deviceClass->id == deviceClassID) {
            deviceClass->addInstance(std::move(consumerPort), std::move(consumerData));
            return;
        }
    }

    std::cout << "Unsupported device class: " << deviceClassID << std::endl;
}

const char *Device::name() {
    return driver->name();
}

void Device::handleMessage(const zagtos::Event &event) {
    std::cout << "Message from " << driver->name() << std::endl;

    if (event.messageType() == zagtos::driver::MSG_START_RESULT) {
        bool result;
        try {
            zbon::decode(event.messageData(), result);
        } catch(zbon::DecoderException &e) {
            std::cout << "Received malformed MSG_START_RESULT message." << std::endl;
            return;
        }

        if (result) {
            std::cout << driver->name() << " startup complete. TODO: what's next?" << std::endl;
        } else {
            std::cout << driver->name() << " startup failed." << std::endl;
        }
    } else if (event.messageType() == zagtos::driver::MSG_FOUND_DEVICE) {
        std::tuple<zagtos::UUID, uint64_t, zbon::EncodedData> msg;
        try {
            zbon::decode(event.messageData(), msg);
        } catch(zbon::DecoderException &e) {
            std::cout << "Received malformed MSG_FOUND_DEVICE message." << std::endl;
            return;
        }
        foundChildDevice(std::get<0>(msg),
                         std::get<1>(msg),
                         std::move(std::get<2>(msg)));
    } else if (event.messageType() == zagtos::driver::MSG_FOUND_CLASS_DEVICE) {
        std::tuple<zagtos::UUID, zagtos::RemotePort, zagtos::MessageData> msg;
        try {
            zbon::decode(event.messageData(), msg);
        } catch(zbon::DecoderException &e) {
            std::cout << "Received malformed MSG_FOUND_CLASS_DEVICE message." << std::endl;
            return;
        }
        auto [deviceClass, consumerPort, consumerData] = std::move(msg);
        foundClassDevice(deviceClass, std::move(consumerPort), std::move(consumerData));
    } else {
        std::cout << "Got message of unknown type from DeviceDriver" << std::endl;
    }
}
