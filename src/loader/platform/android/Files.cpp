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
    size_t initrdAddress = initrdStartProperty->getInt<size_t>();
    const size_t zbonHeaderSize = 9;
    LittleEndian<uint64_t> kernelSize;
    memcpy(&kernelSize, reinterpret_cast<const void *>(initrdAddress + 1), sizeof(uint64_t));
    kernelImageAddress = initrdAddress;
    processImageAddress = initrdAddress + zbonHeaderSize + kernelSize;
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
