#include "Driver.hpp"
#include "Device.hpp"
#include "ClassDevice.hpp"
#include "EmbeddedDrivers.hpp"
#include "PortListener.hpp"
#include <iostream>
#include <zagtos/ZBON.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/Firmware.hpp>

static constexpr zagtos::UUID MSG_BE_INIT(
        0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
        0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55);

int main() {
    zagtos::CheckRunMessageType(MSG_BE_INIT);

    std::cout << "Setup..." << std::endl;

    RegisterEmbeddedDrivers();

    std::cout << "Starting HAL..." << std::endl;

    zagtos::FirmwareInfo firmwareInfo = zagtos::GetFirmwareInfo();
    if (firmwareInfo.type == zagtos::cApi::ZAGTOS_FIRMWARE_TYPE_ACPI) {
        std::cout << "Detected firmware type: ACPI" << std::endl;
        /* create root device (ACPIHAL) */
        DeviceTree = std::make_unique<Device>(DriverRegistry[0]);
    } else if (firmwareInfo.type == zagtos::cApi::ZAGTOS_FIRMWARE_TYPE_DTB) {
        std::cout << "Detected firmware type: DTB" << std::endl;
        /* create root device (DeviceTreeHAL) */
        DeviceTree = std::make_unique<Device>(DriverRegistry[1]);
    } else {
        throw std::runtime_error("Unknown firmware type");
    }

    std::cout << "Starting StorageEngine..." << std::endl;
    StartStorageEngine();

    while (true) {
        zagtos::Event event = zagtos::DefaultEventQueue.waitForEvent();
        PortListener *sender = reinterpret_cast<PortListener *>(event.tag());
        sender->handleMessage(event);
    }
}
