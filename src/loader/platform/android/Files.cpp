#include <Files.hpp>
#include <DeviceTree.hpp>
#include <iostream>
#include <common/utils.hpp>

static size_t kernelImageAddress = 0;
static size_t processImageAddress = 0;
static bool initrdFound = false;

void findInitrd() {
    if (initrdFound) {
        return;
    }
    deviceTree::Tree tree;
    auto chosenNode = tree.rootNode.findChildNode("chosen");
    if (!chosenNode) {
        cout << "could not find /chosen node" << endl;
        Panic();
    }
    auto initrdStartProperty = chosenNode->findProperty("linux,initrd-start");
    if (!initrdStartProperty) {
        cout << "could not find linux,initrd-start property" << endl;
        Panic();
    }
    // TODO: use #address-cells?
    uint64_t initrdAddress = initrdStartProperty->getInt<uint32_t>(0, 2);
    initrdAddress = (initrdAddress << 32) | initrdStartProperty->getInt<uint32_t>(1, 2);
    //size_t initrdAddress = initrdStartProperty->getInt<size_t>();

    const size_t headerSize = 1+3*8;
    LittleEndian<uint64_t> kernelPropertiesSize;
    /* check ZBON type ID for OBJECT (3) */
    assert(*reinterpret_cast<const uint8_t *>(initrdAddress) == 3);
    memcpy(&kernelPropertiesSize, reinterpret_cast<const void *>(initrdAddress + 17), sizeof(uint64_t));
    const size_t kernelSize = headerSize + kernelPropertiesSize;
    kernelImageAddress = initrdAddress;
    processImageAddress = initrdAddress + kernelSize;
    /* check ZBON type ID for OBJECT (3) */
    assert(*reinterpret_cast<const uint8_t *>(processImageAddress) == 3);
    initrdFound = true;
}

void *LoadKernelImage() {
    findInitrd();
    return reinterpret_cast<void *>(kernelImageAddress);
}

void *LoadProcessImage() {
    findInitrd();
    return reinterpret_cast<void *>(processImageAddress);
}
