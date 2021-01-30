#include <zagtos/Messaging.hpp>
#include <sys/mman.h>
#include "SegmentGroup.hpp"

size_t SegmentGroup::numBuses() const {
    return info.busEnd - info.busStart;
}

size_t SegmentGroup::numSpaces() const {
    return numBuses() * DEVICES_PER_BUS * FUNCTIONS_PER_DEVICE;
}

ConfigSpace *SegmentGroup::configSpaceAt(size_t bus, size_t device, size_t function) {
    return configSpaces
            + bus * DEVICES_PER_BUS * FUNCTIONS_PER_DEVICE
            + device * FUNCTIONS_PER_DEVICE
            + function;
}

SegmentGroup::SegmentGroup(zagtos::pci::SegmentGroup info) :
        info{info} {
    auto mem = zagtos::SharedMemory::Physical(info.configBase, numSpaces() * sizeof(ConfigSpace));
    configSpaces = mem.map<ConfigSpace>(PROT_READ|PROT_WRITE);
}

SegmentGroup::~SegmentGroup() {
    zagtos::UnmapWhole(configSpaces);
}

void SegmentGroup::detectDevices(std::vector<Device> &allDevices) {
    for (size_t bus = 0; bus < numBuses(); bus++) {
        for (size_t device = 0; device < DEVICES_PER_BUS; device++) {
            for (size_t function = 0; function < FUNCTIONS_PER_DEVICE; function++) {

                ConfigSpace *configSpace = configSpaceAt(bus, device, function);
                if (configSpace->vendorID() != 0xffff) {
                    if (configSpace->headerType() == 0) {
                        /* regular device */
                        allDevices.emplace_back(configSpace);
                    }
                }
            }
        }
    }
}
