#include <Firmware.hpp>

extern "C" size_t DeviceTreeAddress;

hos_v1::FirmwareType GetFirmwareType() {
    return hos_v1::FirmwareType::DTB;
}

PhysicalAddress GetFirmwareRoot() {
    return {DeviceTreeAddress};
}
