#pragma once

#include <zagtos/protocols/Pci.hpp>
#include "ConfigSpace.hpp"

class Device {
private:
    static const uint32_t MAPPING_IO = 0b1;
    static const uint32_t MEMORY_TYPE_32BIT = 0b0;
    static const uint32_t MEMORY_TYPE_64BIT = 0b100;
    static const uint32_t MAPPING_MASK = 0b001;
    static const uint32_t MEMORY_TYPE_MASK = 0b110;
    static const uint32_t MEMORY_ADDRESS_MASK = 0xfffffff0;
    static const uint32_t GET_LENGTH_COMMAND = 0xffffffff;

    static const uint32_t CAPABILITY_MSI = 0x05;
    static const uint32_t CAPABILITY_MSIX = 0x11;

    zagtos::pci::Device info;
    ConfigSpace *configSpace;
    zagtos::Port _driverPort;

    MSICapability *MSI = nullptr;
    MSIXCapability *MSIX = nullptr;

    /*bool powerManagement;
    bool MSI;
    struct {
        bool available = false;

    } MSI_X;*/

    void detectBARs();
    void detectCapablities();

public:
    Device(ConfigSpace *configSpace);

    uint64_t combinedID() const;
    zbon::EncodedData driverRunMessage();
    zagtos::Port &driverPort();
};
