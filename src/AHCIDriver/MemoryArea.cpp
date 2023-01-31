#include "MemoryArea.hpp"
#include "Registers.hpp"
#include "LogicalCommand.hpp"
#include "Device.hpp"
#include "Port.hpp"
#include <zagtos/protocols/BlockDevice.hpp>

using namespace zagtos;

MemoryArea::MemoryArea(Device &device, size_t length) :
        length{length},
        device{device} {
    std::tie(sharedMemory, deviceAddresses) = SharedMemory::DMA(MaximumDMAAddress, length);
}

void MemoryArea::handleEvent(const Event &event) {
    blockDevice::receive::SubmitAction message;
    if (event.messageType() == message.uuid) {
        try {
            zbon::decode(event.messageData(), message);
        } catch (zbon::DecoderException &) {
            std::cerr << "Got malformed SubmitAction message" << std::endl;
            return;
        }
        std::cout << "Got message action " << message.action << " cookie " << message.cookie << " startSector " << message.startSector << " numSectors " << message.numSectors << " startPage " << message.startPage << std::endl;

        /* TODO: validate input */

        LogicalCommand::Action action;
        switch(message.action) {
        case blockDevice::ACTION_READ:
            action = LogicalCommand::Action::READ;
            break;
        case blockDevice::ACTION_WRITE:
            action = LogicalCommand::Action::WRITE;
            break;
        default:
        {
            blockDevice::send::ActionComplete response{
                .result = blockDevice::ACTION_RESULT_INVALID,
                .cookie = message.cookie,
            };
            message.responsePort.sendMessage(response.uuid, zbon::encode(response));
            return;
        }}

        auto logicalCommand = std::make_unique<LogicalCommand>(LogicalCommand{
            .memoryArea = this,
            .action = action,
            .cookie = message.cookie,
            .startSector = message.startSector,
            .startPage = message.startPage,
            .numSectors = message.numSectors,
            .responsePort = std::move(message.responsePort),
        });
        device.port.submitLogicalCommand(std::move(logicalCommand));
    } else {
        std::cerr << "Unexpected message type on MemoryArea port" << std::endl;
    }
}

void MemoryArea::commandComplete(LogicalCommand &command) {
    blockDevice::send::ActionComplete completeMessage{
        .result = blockDevice::ACTION_RESULT_OK,
        .cookie = command.cookie,
    };
    command.responsePort.sendMessage(completeMessage.uuid, zbon::encode(completeMessage));
}
