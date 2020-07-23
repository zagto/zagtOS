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
    uint32_t classCodeSubclassProgIFRevisionID;
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
                             PROT_READ,
                             MAP_SHARED|MAP_PHYSICAL,
                             -1,
                             segmentGroup.configBase);
        configSpace = reinterpret_cast<FunctionConfigSpace *>(address);
    }
};

std::vector<MappedSegmentGroup> segments;

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
                    uint32_t classC = config->classCodeSubclassProgIFRevisionID;
                    if ((vendorDevice >> 16) != 0xffff) {
                        std::cout << "found PCI device: vendor/device: " << std::hex << vendorDevice
                             << ", class: " << (classC >> 16) << std::endl;
                    }
                }
            }
        }
    }
}
