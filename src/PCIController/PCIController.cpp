#include <iostream>
#include <tuple>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/Controller.hpp>
#include <zagtos/PCI.hpp>

using namespace zagtos;
using namespace zagtos::pci;

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

class MappedSegmentGroup {
//private:
public:
    static const size_t DEVICES_PER_BUS = 1u << 5;
    static const size_t FUNCTIONS_PER_DEVICE = 1u << 3;


    SegmentGroup segmentGroup;
    volatile FunctionConfigSpace *configSpace;

    size_t numBuses() const {
        return segmentGroup.busEnd - segmentGroup.busStart;
    }
    size_t numSpaces() const {
        return numBuses() * DEVICES_PER_BUS * FUNCTIONS_PER_DEVICE;
    }
    volatile FunctionConfigSpace *functionConfigSpace(size_t bus, size_t device, size_t function) {
        return configSpace
                + bus * DEVICES_PER_BUS * FUNCTIONS_PER_DEVICE
                + device * FUNCTIONS_PER_DEVICE
                + function;
    }

public:
    MappedSegmentGroup(SegmentGroup segmentGroup) :
            segmentGroup{segmentGroup} {
        SharedMemory mem(segmentGroup.configBase, numSpaces() * sizeof(FunctionConfigSpace));
        void *address = mem.map(PROT_READ | PROT_WRITE);
        configSpace = reinterpret_cast<FunctionConfigSpace *>(address);
    }
};

std::vector<MappedSegmentGroup> segments;


void handlePotentialDevice(RemotePort &port, volatile FunctionConfigSpace *config) {
    uint32_t classCode = config->classCode();
    uint32_t vendorDevice = config->vendorDevice;
    uint64_t highPart = config->classCodeProgIFRevisionID;
    uint64_t combinedID = (highPart << 32u) | vendorDevice;

    std::cout << "found PCI device: vendor/device: " << std::hex << vendorDevice
         << ", class: " << classCode
         << ", progIF: " << static_cast<uint16_t>(config->progIF()) << std::endl;

    Device dev;
    dev.deviceID = combinedID;

    bool hasAnyMemoryBAR{false};

    for (size_t index = 0; index < 6; index++) {
        static const uint32_t MAPPING_IO = 0b1;
        static const uint32_t MEMORY_TYPE_32BIT = 0b0;
        static const uint32_t MEMORY_TYPE_64BIT = 0b100;
        static const uint32_t MAPPING_MASK = 0b001;
        static const uint32_t MEMORY_TYPE_MASK = 0b110;
        static const uint32_t MEMORY_ADDRESS_MASK = 0xfffffff0;
        static const uint32_t GET_LENGTH_COMMAND = 0xffffffff;

        uint64_t address{0}, length{0};
        uint32_t originalValue = config->BAR[index];
        if ((originalValue & MAPPING_MASK) == MAPPING_IO) {
            std::cout << "BAR " << index
                      << "is an I/O port mapped BAR. ignoring." << std::endl;
        } else { /* Memory mapped */
            hasAnyMemoryBAR = true;
            switch (originalValue & MEMORY_TYPE_MASK) {
            case MEMORY_TYPE_32BIT:
                address = originalValue & MEMORY_ADDRESS_MASK;

                config->BAR[index] = GET_LENGTH_COMMAND;
                length = (~(config->BAR[index] & MEMORY_ADDRESS_MASK)) + 1;
                config->BAR[index] = originalValue;

                break;
            case MEMORY_TYPE_64BIT: {
                if (index == 5) {
                    std::cout << "64-bit BAR at end of BAR registers." << std::endl;
                    return;
                }
                uint64_t highOriginalValue = config->BAR[index + 1];
                address = (highOriginalValue << 32) | (originalValue & MEMORY_ADDRESS_MASK);

                config->BAR[index] = GET_LENGTH_COMMAND;
                config->BAR[index + 1] = GET_LENGTH_COMMAND;
                length = (~((static_cast<uint64_t>(config->BAR[index + 1]) << 32)
                            | (config->BAR[index] & MEMORY_ADDRESS_MASK))) + 1;
                config->BAR[index] = originalValue;
                config->BAR[index + 1] = highOriginalValue;

                /* a 64-bit BAR also takes up the following register. Don't try to process it as a
                 * standalone BAR in next iteration */
                index++;
                break;
            }
            default:
                std::cout << "Ignoring BAR " << index << " of unknown memory type" << std::endl;
            }
        }
        if (length != 0) {
            std::cout << "BAR " << index << ": address " << address
                      << " length " << length << std::endl;
            if (address <= std::numeric_limits<size_t>::max()
                && address + length > address
                && static_cast<size_t>(address + length) == address + length) {

                BaseRegister BAR{SharedMemory(address, length),
                                 length};
                dev.BAR[index].emplace(std::move(BAR));
            } else {
                std::cout << "BAR out of address space. ignoring." << std::endl;
            }
        }
    }

    if (!hasAnyMemoryBAR) {
        std::cout << "did not find any memory BAR" << std::endl;
        return;
    }
    config->setCommand(Command::BusMasterEnable);
    config->setCommand(Command::MemorySpaceEnable);
    /* avoid sending any legacy interrupts - we only support MSI-X */
    config->setCommand(Command::InterruptDisable);

    dev.capabilites = config->findCapabilities();

    zbon::EncodedData driverData = zbon::encode(dev);
    port.sendMessage(MSG_FOUND_DEVICE,
                     zbon::encode(std::tuple(combinedID,
                                             std::move(driverData))));

}


int main() {
    auto [envPort, data] = decodeRunMessage<std::tuple<RemotePort, zbon::EncodedData>>(MSG_START_CONTROLLER);
    std::vector<SegmentGroup> segmentGroups;
    zbon::decode(data, segmentGroups);

    std::cout << "Starting PCI Controller..." << std::endl;
    std::cout << "Got " << segmentGroups.size() << " segment groups" << std::endl;
    segments = std::vector<MappedSegmentGroup>(segmentGroups.begin(), segmentGroups.end());

    for (MappedSegmentGroup &segment: segments) {
        for (size_t bus = 0; bus < segment.numBuses(); bus++) {
            for (size_t device = 0; device < MappedSegmentGroup::DEVICES_PER_BUS; device++) {
                for (size_t function = 0; function < MappedSegmentGroup::FUNCTIONS_PER_DEVICE; function++) {
                    volatile FunctionConfigSpace *config = segment.functionConfigSpace(bus, device, function);
                    uint32_t vendorDevice = config->vendorDevice;
                    if ((vendorDevice >> 16) != 0xffff) {
                        if (config->headerType() == 0) {
                            handlePotentialDevice(envPort, config);
                        }
                    }
                }
            }
        }
    }
}
