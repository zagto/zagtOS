#include <Firmware.hpp>
#include <DeviceTree.hpp>

extern "C" size_t DeviceTreeAddress;

hos_v1::FirmwareInfo GetFirmwareInfo() {
    deviceTree::Tree tree;
    return {
        .type = hos_v1::FirmwareType::DTB,
        .rootAddress = DeviceTreeAddress,
        .regionLength = tree.memoryRegion().length
    };
}
