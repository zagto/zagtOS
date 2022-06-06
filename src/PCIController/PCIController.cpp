#include <iostream>
#include <tuple>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Pci.hpp>
#include <zagtos/protocols/Driver.hpp>
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
    auto [controllerType, envPort, segmentGroupsInfo] =
            decodeRunMessage<std::tuple<zagtos::UUID, RemotePort, std::vector<pci::SegmentGroup>>>(driver::MSG_START);

    std::cout << "Starting PCI Controller..." << std::endl;
    std::cout << "Got " << segmentGroupsInfo.size() << " segment groups" << std::endl;
    std::vector<SegmentGroup> segments(segmentGroupsInfo.begin(), segmentGroupsInfo.end());

    std::vector<Device> devices;

    for (SegmentGroup &segment: segments) {
        segment.detectDevices(devices);
    }

    for (Device &device: devices) {
        envPort.sendMessage(driver::MSG_FOUND_DEVICE,
                            zbon::encodeObject(driver::CONTROLLER_TYPE_PCI,
                                               device.combinedID(),
                                               device.driverRunMessage()));
    }

    std::vector<std::reference_wrapper<Port>> driverPorts;

    for (auto &dev: devices) {
        driverPorts.emplace_back(dev.driverPort());
    }

    while (true) {
        auto messageInfo = Port::receiveMessage(driverPorts);
        if (messageInfo->type == pci::MSG_ALLOCATE_MSI_IRQ) {
            zagtos::RemotePort responsePort;
            try {
                zbon::decode(messageInfo->data, responsePort);
            } catch (zbon::DecoderException *e) {
                std::cerr << "received malformed ALLOCATE_MSI_IRQ message from driver" << std::endl;
                continue;
            }

            size_t portIndex = messageInfo->portIndex;
            std::cerr << "register interrupt for device " << portIndex << std::endl;
            auto interrupt = devices[portIndex].allocateMSIInterrupt();

            responsePort.sendMessage(pci::MSG_ALLOCATE_MSI_IRQ_RESULT, zbon::encode(interrupt));
        } else {
            std::cerr << "received unknown message type from driver" << std::endl;
        }
    }
}
