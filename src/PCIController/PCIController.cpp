#include <iostream>
#include <tuple>
#include <sys/mman.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/Controller.hpp>
#include <zagtos/PCI.hpp>

using namespace zagtos;
using namespace zagtos::pci;

/* This assumes little-endian everywhere */

struct alignas(0x1000) FunctionConfigSpace {
    uint32_t vendorDevice;
    uint32_t commandStatus;
    uint32_t classCodeProgIFRevisionID;
    uint32_t BISTHeaderTypeLatencyTimerCacheLineSize;
    uint32_t BAR[6];

    uint16_t classCode() volatile {
        return classCodeProgIFRevisionID >> 16;
    }
    uint8_t progIF() volatile {
        return classCodeProgIFRevisionID >> 8;
    }
    uint8_t headerType() volatile {
        return BISTHeaderTypeLatencyTimerCacheLineSize >> 16;
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
        void *address = mmap(nullptr,
                             numSpaces() * sizeof(FunctionConfigSpace),
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED|MAP_PHYSICAL,
                             -1,
                             segmentGroup.configBase);
        configSpace = reinterpret_cast<FunctionConfigSpace *>(address);
    }
};

std::vector<MappedSegmentGroup> segments;


void notifyEnvironmentOfDevice(RemotePort &port, volatile FunctionConfigSpace *config) {
    uint32_t classCode = config->classCode();
    uint32_t vendorDevice = config->vendorDevice;
    uint64_t highPart = config->classCodeProgIFRevisionID;
    uint64_t combinedID = (highPart << 32u) | vendorDevice;

    std::cout << "found PCI device: vendor/device: " << std::hex << vendorDevice
         << ", class: " << classCode
         << ", progIF: " << static_cast<uint16_t>(config->progIF()) << std::endl;

    Device dev;
    dev.deviceID = combinedID;
    for (size_t index = 0; index < 5; index++) {
        static const uint32_t MAPPING_IO = 0b1;
        static const uint32_t MEMORY_TYPE_32BIT = 0b0;
        static const uint32_t MEMORY_TYPE_64BIT = 0b100;
        static const uint32_t MAPPING_MASK = 0b001;
        static const uint32_t MEMORY_TYPE_MASK = 0b110;
        static const uint32_t MEMORY_ADDRESS_MASK = 0xfffffff0;
        static const uint32_t GET_LENGTH_COMMAND = 0xffffffff;

        uint64_t address{0}, length{0};
        uint32_t originalValue = config->BAR[index];
        std::cout << "originalValue " << originalValue << std::endl;
        if ((originalValue & MAPPING_MASK) == MAPPING_IO) {
            std::cout << "BAR " << index
                      << "is an I/O port mapped BAR. ignoring." << std::endl;
        } else { /* Memory mapped */
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

                BaseRegister BAR{SharedMemory(MAP_PHYSICAL, address, length),
                                 length};
                dev.BAR[index].emplace(std::move(BAR));
            } else {
                std::cout << "BAR out of address space. ignoring." << std::endl;
            }
        }
    }

    zbon::EncodedData driverData = zbon::encode(dev);
    port.sendMessage(MSG_FOUND_DEVICE,
                     zbon::encode(std::tuple(combinedID,
                                             std::move(driverData))));

}


int main() {
    auto [envPort, data] = decodeRunMessage<std::tuple<RemotePort, zbon::EncodedData>>(MSG_START_CONTROLLER);
    std::vector<SegmentGroup> segmentGroups;
    if (!zbon::decode(data, segmentGroups)) {
        throw new std::logic_error("Got malformd segment group info.");
    }

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
                            notifyEnvironmentOfDevice(envPort, config);
                        }
                    }
                }
            }
        }
    }
}
