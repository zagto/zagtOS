#include <iostream>
#include <tuple>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/Controller.hpp>
#include <zagtos/PCI.hpp>
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

struct alignas(0x1000) FunctionConfigSpace {
    uint32_t vendorDevice;
    uint32_t commandStatus;
    uint32_t classCodeProgIFRevisionID;
    uint32_t BISTHeaderTypeLatencyTimerCacheLineSize;
    uint32_t BAR[6];
    uint32_t subsystemID;
    uint32_t expansionROMBase;
    uint32_t capabilitiesPointer;
    uint32_t reserved;
    uint32_t maxLatencyMinGrantInterruptInfo;

    uint32_t unused0[0x100 - 0x40];
    uint32_t firstExtendedCapability;

    uint16_t classCode() volatile {
        return classCodeProgIFRevisionID >> 16;
    }
    uint8_t progIF() volatile {
        return classCodeProgIFRevisionID >> 8;
    }
    uint8_t headerType() volatile {
        return BISTHeaderTypeLatencyTimerCacheLineSize >> 16;
    }
    void setCommand(uint16_t setBits) volatile {
        uint32_t value = commandStatus;
        value |= static_cast<uint32_t>(setBits) << 16;
        commandStatus = value;
    }
    void clearCommand(uint16_t clearBits) volatile {
        uint32_t value = commandStatus;
        value &= ~(static_cast<uint32_t>(clearBits) << 16);
        commandStatus = value;
    }
    bool checkStatus(uint16_t statusBit) volatile {
        return commandStatus & statusBit;
    }
    uint32_t readRegister(size_t offset) volatile {
        return *reinterpret_cast<uint32_t *>(reinterpret_cast<size_t>(this) + offset);
    }
    std::vector<uint8_t> findCapabilities() volatile {
        if (!checkStatus(Status::HasCapabilitiesList)) {
            return {};
        }

        static const uint8_t pointerMask = 0xfc;
        uint8_t pointer = capabilitiesPointer & pointerMask;
        std::vector<uint8_t> result;
        while (pointer != 0) {
            uint32_t reg = readRegister(pointer);
            uint8_t id = (reg >> 8) & 0xff;
            if (id != 0) {
                /* NULL capability is ignored */
                result.push_back(id);
            }
            pointer = reg & pointerMask;
        }
        return result;
    }
};

int main() {
    auto [envPort, segmentGroupsInfo] = decodeRunMessage<std::tuple<RemotePort, std::vector<pci::SegmentGroup>>>(MSG_START_CONTROLLER);

    std::cout << "Starting PCI Controller..." << std::endl;
    std::cout << "Got " << segmentGroupsInfo.size() << " segment groups" << std::endl;
    std::vector<SegmentGroup> segments(segmentGroupsInfo.begin(), segmentGroupsInfo.end());

    std::vector<Device> devices;

    for (SegmentGroup &segment: segments) {
        segment.detectDevices(devices);
    }

    for (Device &device: devices) {
        envPort.sendMessage(MSG_FOUND_DEVICE,
                            zbon::encodeObject(device.combinedID(),
                                               device.driverRunMessage()));
    }
}
