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

    while (true) {
        Event event = DefaultEventQueue.waitForEvent();
        assert(event.isMessage());
        size_t deviceID = event.tag();
        if (event.messageType() == pci::MSG_ALLOCATE_MSI_IRQ) {
            zagtos::RemotePort responsePort;
            try {
                zbon::decode(event.messageData(), responsePort);
            } catch (zbon::DecoderException *e) {
                std::cerr << "received malformed ALLOCATE_MSI_IRQ message from driver" << std::endl;
                continue;
            }
            auto interrupt = devices[deviceID].allocateMSIInterrupt();
            responsePort.sendMessage(pci::MSG_ALLOCATE_MSI_IRQ_RESULT, zbon::encode(interrupt));
        } else if (event.messageType() == pci::MSG_READ_CONFIG_SPACE) {
            std::tuple<zagtos::RemotePort, uint32_t> message;
            try {
                zbon::decode(event.messageData(), message);
            } catch (zbon::DecoderException *e) {
                std::cerr << "received malformed MSG_READ_CONFIG_SPACE message from driver" << std::endl;
                continue;
            }
            auto [responsePort, registerIndex] = std::move(message);
            std::optional<uint32_t> result;
            if (registerIndex < ConfigSpace::NUM_REGISERS) {
                result = devices[deviceID].readConfigSpace(registerIndex);
            }
            responsePort.sendMessage(pci::MSG_READ_CONFIG_SPACE_RESULT, zbon::encode(result));
        } else if (event.messageType() == pci::MSG_WRITE_CONFIG_SPACE) {
            std::tuple<zagtos::RemotePort, uint32_t, uint32_t> message;
            try {
                zbon::decode(event.messageData(), message);
            } catch (zbon::DecoderException *e) {
                std::cerr << "received malformed MSG_WRITE_CONFIG_SPACE message from driver" << std::endl;
                continue;
            }
            auto [responsePort, registerIndex, value] = std::move(message);
            bool ok = registerIndex < ConfigSpace::NUM_REGISERS;
            if (ok) {
                devices[deviceID].writeConfigSpace(registerIndex, value);
            }
            responsePort.sendMessage(pci::MSG_WRITE_CONFIG_SPACE_RESULT, zbon::encode(ok));
        } else {
            std::cerr << "received unknown message type from driver" << std::endl;
        }
    }
}
