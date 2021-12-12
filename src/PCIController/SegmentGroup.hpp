#pragma once

#include <zagtos/protocols/Pci.hpp>
#include "ConfigSpace.hpp"
#include "Device.hpp"

class SegmentGroup {
public:
    static const size_t DEVICES_PER_BUS = 1u << 5;
    static const size_t FUNCTIONS_PER_DEVICE = 1u << 3;

private:
    zagtos::pci::SegmentGroup info;
    ConfigSpace *configSpaces;

    size_t numBuses() const;
    size_t numSpaces() const;
    ConfigSpace *configSpaceAt(size_t bus, size_t device, size_t function);

public:
    SegmentGroup(zagtos::pci::SegmentGroup segmentGroup);
    SegmentGroup(SegmentGroup &other) = delete;
    ~SegmentGroup();

    void detectDevices(std::vector<Device> &allDevices);
};
