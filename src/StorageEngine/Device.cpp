#include "Device.hpp"
#include "DeviceList.hpp"
#include <zagtos/protocols/BlockDevice.hpp>
#include <sys/mman.h>
#include <cstring>

Device::Device(zagtos::RemotePort devicePort,
               uint64_t deviceID,
               zagtos::blockDevice::DeviceInfo info):
        devicePort(std::move(devicePort)),
        info{info},
        id{deviceID} {
    std::cout << "New " << (info.blockSize * info.numBlocks / (1024 * 1024)) << " MiB device"
              << std::endl;
    sendAllocateHeader();
}

void Device::sendAllocateHeader() {
    zagtos::blockDevice::send::Allocate message {
        .length = info.blockSize,
        .responsePort = port,
    };
    devicePort.sendMessage(message.uuid, zbon::encode(message));
    state = State::ALLOCATE_HEADER;
}

void Device::sendLoadHeader() {
    zagtos::blockDevice::send::Read message {
        .startPage = 0,
        .startSector = 1,
        .numSectors = 1,
        .responsePort = port,
    };
    gptHeaderPort.sendMessage(message.uuid, zbon::encode(message));
    state = State::LOAD_HEADER;
}

void Device::sendAllocatePartitions() {
    zagtos::blockDevice::send::Allocate message {
        .length = info.blockSize * numPartitionSectors,
        .responsePort = port,
    };
    devicePort.sendMessage(message.uuid, zbon::encode(message));
    state = State::ALLOCATE_PARTITIONS;
}

void Device::sendLoadPartitions() {
    zagtos::blockDevice::send::Read message {
        .startPage = 0,
        .startSector = gptHeader->firstPartitionEntryLba,
        .numSectors = numPartitionSectors,
        .responsePort = port,
    };
    gptPartitionsPort.sendMessage(message.uuid, zbon::encode(message));
    state = State::LOAD_PARTITIONS;
}

void Device::handleEvent(const zagtos::Event &event) {
    try {
        switch(state) {
        case State::ALLOCATE_HEADER: {
            zagtos::blockDevice::receive::AllocateResult message;
            assert(message.uuid == event.messageType()); // TODO
            zbon::decode(event.messageData(), message);
            gptHeaderMemory = std::move(message.sharedMemory);
            gptHeaderPort = std::move(message.port);
            sendLoadHeader();
            break;
        }
        case State::LOAD_HEADER: {
            zagtos::blockDevice::receive::ReadResult message;
            assert(message.uuid == event.messageType()); // TODO
            zbon::decode(event.messageData(), message);
            assert(message.success == true);
            gptHeader = gptHeaderMemory.map<const GptHeader>(PROT_READ);

            const char *expectedSignature = "EFI PART";
            if (memcmp(expectedSignature, gptHeader->signature, 8) != 0) {
                std::cout << "signature does not match, ignoring device!" << std::endl;
                state = State::NO_GPT;
                break;
            }
            std::cout << "signature matches! found EFI partition table" << std::endl;
            assert(gptHeader->firstUsableLba > gptHeader->firstPartitionEntryLba); // TODO

            std::cout << "firstPartitionEntryLba: " << gptHeader->firstPartitionEntryLba << std::endl;
            std::cout << "firstUsableLba: " << gptHeader->firstUsableLba << std::endl;
            std::cout << "numPartitionEntries: " << gptHeader->numPartitionEntries << std::endl;
            std::cout << "partitionEntriesSize: " << gptHeader->partitionEntriesSize << std::endl;

            numPartitionSectors = gptHeader->firstUsableLba - gptHeader->firstPartitionEntryLba;

            sendAllocatePartitions();
            break;
        }
        case State::ALLOCATE_PARTITIONS: {
            zagtos::blockDevice::receive::AllocateResult message;
            assert(message.uuid == event.messageType()); // TODO
            zbon::decode(event.messageData(), message);
            gptPartitionsMemory = std::move(message.sharedMemory);
            gptPartitionsPort = std::move(message.port);
            sendLoadPartitions();
            break;
        }
        case State::LOAD_PARTITIONS: {
            zagtos::blockDevice::receive::ReadResult message;
            assert(message.uuid == event.messageType()); // TODO
            zbon::decode(event.messageData(), message);
            assert(message.success == true);

            auto entry = gptPartitionsMemory.map<const PartitionEntry>(PROT_READ);
            partitionEntries.resize(gptHeader->numPartitionEntries);

            for (size_t index = 0; index < gptHeader->numPartitionEntries; index++) {
                uint64_t start = entry->firstLba * info.blockSize / 1'000'000;
                uint64_t length = (entry->lastLba - entry->firstLba) * info.blockSize / 1'000'000;
                if (entry->partitionType.low != 0 || entry->partitionType.high != 0) {
                    std::cout << "partition entry " << (index+1) << " start " << start << " MiB, length " << length << " MiB" << std::endl;
                }

                partitionEntries[index] = entry;
                entry = reinterpret_cast<const PartitionEntry *>(reinterpret_cast<size_t>(entry) + gptHeader->partitionEntriesSize);
            }
            break;
        }
        default:
            std::cout << "TODO" << std::endl;
        }
    } catch (zbon::DecoderException &decoderException) {
        std::cerr << "Got invalid message from device" << std::endl;
    }
}
