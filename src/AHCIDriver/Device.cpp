#include "Device.hpp"
#include "MemoryArea.hpp"
#include <zagtos/protocols/BlockDevice.hpp>

using namespace zagtos;

Device::Device(uint64_t sectorSize, uint64_t numSectors, ::Port &port) :
    sectorSize{sectorSize},
    numSectors{numSectors},
    port{port} {}

void Device::handleEvent(const Event &event) {
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

    auto memoryArea = std::make_unique<MemoryArea>(*this, message.length);
    blockDevice::send::AllocateResult response = {
        memoryArea->sharedMemory,
        memoryArea->port,
    };
    message.responsePort.sendMessage(response.uuid, zbon::encode(response));
    memoryAreas.insert(std::move(memoryArea));
}
