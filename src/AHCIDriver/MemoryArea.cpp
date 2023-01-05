#include "MemoryArea.hpp"
#include "Registers.hpp"
#include <zagtos/protocols/BlockDevice.hpp>

using namespace zagtos;

MemoryArea::MemoryArea(size_t length) :
        length{length} {
    std::tie(sharedMemory, deviceAddresses) = SharedMemory::DMA(MaximumDMAAddress, length);
}

void MemoryArea::handleMessage(const Event &event) {
    blockDevice::receive::Read readMessage;
    if (event.messageType() == readMessage.uuid) {
        try {
            zbon::decode(event.messageData(), readMessage);
        } catch (zbon::DecoderException &) {
            std::cerr << "Got malformed READ message" << std::endl;
            return;
        }
        std::cout << "Got read message startSector " << readMessage.startSector << " length " << readMessage.length << std::endl;
    } else {
        std::cerr << "Unexpected message type on MemoryArea port" << std::endl;
    }
}
