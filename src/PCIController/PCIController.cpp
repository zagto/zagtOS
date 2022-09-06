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
            auto interrupt = devices[portIndex].allocateMSIInterrupt();
            responsePort.sendMessage(pci::MSG_ALLOCATE_MSI_IRQ_RESULT, zbon::encode(interrupt));
        } else if (messageInfo->type == pci::MSG_READ_CONFIG_SPACE) {
            std::tuple<zagtos::RemotePort, uint32_t> message;
            try {
                zbon::decode(messageInfo->data, message);
            } catch (zbon::DecoderException *e) {
                std::cerr << "received malformed MSG_READ_CONFIG_SPACE message from driver" << std::endl;
                continue;
            }

            auto [responsePort, registerIndex] = std::move(message);
            size_t portIndex = messageInfo->portIndex;
            std::optional<uint32_t> result;
            if (registerIndex < ConfigSpace::NUM_REGISERS) {
                result = devices[portIndex].readConfigSpace(registerIndex);
            }
            responsePort.sendMessage(pci::MSG_READ_CONFIG_SPACE_RESULT, zbon::encode(result));
        } else if (messageInfo->type == pci::MSG_WRITE_CONFIG_SPACE) {
            std::tuple<zagtos::RemotePort, uint32_t, uint32_t> message;
            try {
                zbon::decode(messageInfo->data, message);
            } catch (zbon::DecoderException *e) {
                std::cerr << "received malformed MSG_WRITE_CONFIG_SPACE message from driver" << std::endl;
                continue;
            }

            auto [responsePort, registerIndex, value] = std::move(message);
            size_t portIndex = messageInfo->portIndex;
            bool ok = registerIndex < ConfigSpace::NUM_REGISERS;
            if (ok) {
                devices[portIndex].writeConfigSpace(registerIndex, value);
            }
            responsePort.sendMessage(pci::MSG_WRITE_CONFIG_SPACE_RESULT, zbon::encode(ok));
        } else {
            std::cerr << "received unknown message type from driver" << std::endl;
        }
    }
}
