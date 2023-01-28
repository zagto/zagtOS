#include "MemoryArea.hpp"
#include "Registers.hpp"
#include "Command.hpp"
#include <zagtos/protocols/BlockDevice.hpp>

using namespace zagtos;

MemoryArea::MemoryArea(Device &device, size_t length) :
        length{length},
        device{device} {
    std::tie(sharedMemory, deviceAddresses) = SharedMemory::DMA(MaximumDMAAddress, length);
}

void MemoryArea::handleEvent(const Event &event) {
    blockDevice::receive::Read readMessage;
    if (event.messageType() == readMessage.uuid) {
        try {
            zbon::decode(event.messageData(), readMessage);
        } catch (zbon::DecoderException &) {
            std::cerr << "Got malformed READ message" << std::endl;
            return;
        }
        std::cout << "Got read message startSector " << readMessage.startSector << " numSectors " << readMessage.numSectors << " startPage " << readMessage.startPage << std::endl;

        /* TODO: check free command slots */
        Command cmd(ATACommand::READ_DMA_EXT,
                    readMessage.startSector,
                    readMessage.startPage,
                    readMessage.numSectors,
                    false,
                    this);
        /* TODO: make non-syncronous */
        device.port.executeCommand(cmd, true);

        blockDevice::send::ReadResult resultMessage{true};
        readMessage.responsePort.sendMessage(resultMessage.uuid, zbon::encode(resultMessage));
    } else {
        std::cerr << "Unexpected message type on MemoryArea port" << std::endl;
    }
}
