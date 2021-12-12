#include <iostream>
#include <tuple>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Pci.hpp>
#include <zagtos/protocols/Controller.hpp>
#include "SegmentGroup.hpp"

using namespace zagtos;

/* This assumes little-endian everywhere */

enum Command {
    InterruptDisable = 1<<10,
    BusMasterEnable = 1<<2,
    MemorySpaceEnable = 1<<1,
    IOSpaceEnable = 1<<0,
};

enum Status {
    HasCapabilitiesList = 1<<4,
};

int main() {
    auto [envPort, segmentGroupsInfo] =
            decodeRunMessage<std::tuple<RemotePort, std::vector<pci::SegmentGroup>>>(controller::MSG_START);

    std::cout << "Starting PCI Controller..." << std::endl;
    std::cout << "Got " << segmentGroupsInfo.size() << " segment groups" << std::endl;
    std::vector<SegmentGroup> segments(segmentGroupsInfo.begin(), segmentGroupsInfo.end());

    std::vector<Device> devices;

    for (SegmentGroup &segment: segments) {
        segment.detectDevices(devices);
    }

    for (Device &device: devices) {
        envPort.sendMessage(controller::MSG_FOUND_DEVICE,
                            zbon::encodeObject(device.combinedID(),
                                               device.driverRunMessage()));
    }

    std::vector<std::reference_wrapper<Port>> driverPorts;
    std::transform(devices.begin(),
                   devices.end(),
                   driverPorts.begin(),
                   [](Device &dev) -> std::reference_wrapper<Port> { return dev.driverPort(); });
    while (true) {
        auto messageInfo = Port::receiveMessage(driverPorts);
        if (messageInfo->type == pci::MSG_ALLOCATE_MSI_IRQ) {

        }
    }
}
