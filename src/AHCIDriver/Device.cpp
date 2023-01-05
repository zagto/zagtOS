#include "Device.hpp"
#include "MemoryArea.hpp"
#include <zagtos/protocols/BlockDevice.hpp>

using namespace zagtos;

Device::Device(uint64_t sectorSize, uint64_t numSectors, size_t portID) :
    sectorSize{sectorSize},
    numSectors{numSectors},
    portID{portID} {}

void Device::handleMessage(const Event &event) {
    blockDevice::receive::Allocate message;
    if (event.messageType() != message.uuid) {
        std::cerr << "Unexpected message type received on Device port" << std::endl;
        return;
    }
    try {
        zbon::decode(event.messageData(), message);
    } catch (zbon::DecoderException &e) {
        std::cerr << "received malformed Allocate message" << std::endl;
        return;
    }

    MemoryArea *memoryArea = new MemoryArea(message.length);
    blockDevice::send::AllocateResult response = {
        memoryArea->sharedMemory,
        memoryArea->messagePort,
    };
    message.responsePort.sendMessage(response.uuid, zbon::encode(response));
}
