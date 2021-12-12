#include <zagtos/Messaging.hpp>
#include "Device.hpp"

void Device::detectBARs() {
    for (size_t index = 0; index < 6; index++) {
        uint64_t address{0}, length{0};
        uint32_t originalValue = configSpace->BAR(index);
        if ((originalValue & MAPPING_MASK) == MAPPING_IO) {
            std::cout << "BAR " << index
                      << "is an I/O port mapped BAR. ignoring." << std::endl;
        } else { /* Memory mapped */
            switch (originalValue & MEMORY_TYPE_MASK) {
            case MEMORY_TYPE_32BIT:
                address = originalValue & MEMORY_ADDRESS_MASK;

                configSpace->BAR(index, GET_LENGTH_COMMAND);
                length = (~(configSpace->BAR(index) & MEMORY_ADDRESS_MASK)) + 1;
                configSpace->BAR(index, originalValue);

                break;
            case MEMORY_TYPE_64BIT: {
                if (index == 5) {
                    std::cout << "64-bit BAR at end of BAR registers." << std::endl;
                    return;
                }
                uint64_t highOriginalValue = configSpace->BAR(index + 1);
                address = (highOriginalValue << 32) | (originalValue & MEMORY_ADDRESS_MASK);

                configSpace->BAR(index, GET_LENGTH_COMMAND);
                configSpace->BAR(index + 1, GET_LENGTH_COMMAND);
                length = (~((static_cast<uint64_t>(configSpace->BAR(index + 1)) << 32)
                            | (configSpace->BAR(index) & MEMORY_ADDRESS_MASK))) + 1;
                configSpace->BAR(index, originalValue);
                configSpace->BAR(index + 1, highOriginalValue);

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

                info.BAR[index] = zagtos::SharedMemory::Physical(address, length);
            } else {
                std::cout << "BAR out of address space. ignoring." << std::endl;
            }
        }
    }
}

void Device::detectCapablities() {
    if (!configSpace->hasCapabilitiesList()) {
        return;
    }

    uint32_t index = configSpace->capabilitiesPointer();
    while (index != 0) {
        Capability *cap = reinterpret_cast<Capability *>(configSpace->__register_data + index);
        switch (cap->header.id()) {
        case CAPABILITY_MSI:
            if (MSI != nullptr) {
                std::cout << "MSI capability found twice!" << std::endl;
            }
            MSI = reinterpret_cast<MSICapability *>(cap);
            info.supportedCapabilities.push_back(zagtos::pci::Capability::MSI);
            break;
        case CAPABILITY_MSIX:
            if (MSIX != nullptr) {
                std::cout << "MSI-X capability found twice!" << std::endl;
            }
            MSIX = reinterpret_cast<MSIXCapability *>(cap);
            info.supportedCapabilities.push_back(zagtos::pci::Capability::MSI_X);
            break;
        }

        index = cap->header.nextIndex();
    }
}

Device::Device(ConfigSpace *configSpace) :
    configSpace{configSpace} {

    uint64_t combinedID = static_cast<uint64_t>(configSpace->vendorID())
            | (static_cast<uint64_t>(configSpace->deviceID()) << 16)
            | (static_cast<uint64_t>(configSpace->classCodeProgIFRevisionID()) << 32);
    info.deviceID = combinedID;

    detectBARs();

    configSpace->busMasterEnable(1);
    configSpace->IOSpaceEnable(0);
    configSpace->memorySpaceEnable(1);

    detectCapablities();

    std::cout << "device having MSI: " << (MSI != nullptr) << std::endl;
    std::cout << "device having MSI-X: " << (MSIX != nullptr) << std::endl;

    /* avoid sending any legacy interrupts - we only support MSI / MSI-X */
    configSpace->interruptDisable(1);
}

uint64_t Device::combinedID() const {
    return info.deviceID;
}

zbon::EncodedData Device::driverRunMessage() {
    return zbon::encodeObject(_driverPort, info);
}

zagtos::Port &Device::driverPort() {
    return driverPort();
}
