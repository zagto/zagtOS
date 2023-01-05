#include <iostream>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/StorageEngine.hpp>
#include <zagtos/protocols/ClassDevice.hpp>

using namespace zagtos;

void foundDevice(RemotePort, MessageData deviceData) {
    classDevice::BlockDeviceInfo info;
    try {
        zbon::decode(deviceData, info);
    } catch (zbon::DecoderException &) {
        std::cout << "Received malformed BlockDeviceInfo" << std::endl;
        return;
    }
    std::cout << "Found block device size: " << info.blockSize * info.numBlocks << std::endl;
}

int main() {
    auto subscribePort = decodeRunMessage<RemotePort>(storageEngine::MSG_START);
    Port eventsPort(DefaultEventQueue, 42);
    subscribePort.sendMessage(classDevice::MSG_SUBSCRIBE, zbon::encode(eventsPort));
    Event event = DefaultEventQueue.waitForEvent();
    std::cout << "Received Event! MSG_SUBSCRIBE_RESULT expected" << std::endl;
    if (event.messageType() != classDevice::MSG_SUBSCRIBE_RESULT) {
        throw std::logic_error(
                    "Unexpected message type while waiting for SUBSCRIBE_RESULT message");
    }
    std::tuple<RemotePort, std::vector<RemotePort>, std::vector<zbon::EncodedData>> message;
    auto [managementPort, devicePorts, deviceData] = std::move(message);
    assert(devicePorts.size() == deviceData.size());
    for (size_t index = 0; index < devicePorts.size(); index++) {
        foundDevice(std::move(devicePorts[index]), std::move(deviceData[index]));
    }
    std::cout << "StorageEngine startup complete, waiting for more block devices" << std::endl;
    while (true) {
        zagtos::Event event = zagtos::DefaultEventQueue.waitForEvent();
        assert(event.isMessage());
        assert(event.messageType() == classDevice::MSG_FOUND_DEVICE);

        std::tuple<RemotePort, MessageData> message;
        try {
            zbon::decode(event.messageData(), message);
        } catch (zbon::DecoderException &) {
            std::cout << "Received malformed MSG_FOUND_DEVICE message" << std::endl;
            continue;
        }

        auto [remotePort, deviceData] = std::move(message);
        foundDevice(std::move(remotePort), std::move(deviceData));
    }
}
