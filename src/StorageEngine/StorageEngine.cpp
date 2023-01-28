#include <iostream>
#include <unordered_map>
#include <zagtos/Messaging.hpp>
#include <zagtos/EventListener.hpp>
#include <zagtos/protocols/StorageEngine.hpp>
#include <zagtos/protocols/ClassDevice.hpp>
#include <zagtos/protocols/BlockDevice.hpp>
#include "DeviceList.hpp"

using namespace zagtos;

struct DeviceFoundListener : public EventListener {
    void handleEvent(const zagtos::Event &event) final {
        assert(event.messageType() == classDevice::MSG_FOUND_DEVICE);

        std::tuple<RemotePort, uint64_t, blockDevice::DeviceInfo> message;
        try {
            zbon::decode(event.messageData(), message);
        } catch (zbon::DecoderException &) {
            std::cout << "Received malformed MSG_FOUND_DEVICE message" << std::endl;
            return;
        }

        auto [remotePort, deviceID, info] = std::move(message);
        auto device = std::make_unique<Device>(std::move(remotePort), deviceID, info);
        DeviceList::instance.add(std::move(device));
    }
};

int main() {
    auto subscribePort = decodeRunMessage<RemotePort>(storageEngine::MSG_START);
    DeviceFoundListener deviceFoundListener;
    subscribePort.sendMessage(classDevice::MSG_SUBSCRIBE, zbon::encode(deviceFoundListener.port));
    Event event = DefaultEventQueue.waitForEvent();
    std::cout << "Received Event! MSG_SUBSCRIBE_RESULT expected" << std::endl;
    if (event.messageType() != classDevice::MSG_SUBSCRIBE_RESULT) {
        throw std::logic_error(
                    "Unexpected message type while waiting for SUBSCRIBE_RESULT message");
    }
    std::tuple<RemotePort,
            std::vector<RemotePort>,
            std::vector<uint64_t>,
            std::vector<zbon::EncodedData>> message;
    auto [managementPort, devicePorts, deviceIDs, deviceData] = std::move(message);
    assert(devicePorts.size() == deviceData.size());
    assert(devicePorts.size() == deviceIDs.size());
    for (size_t index = 0; index < devicePorts.size(); index++) {
        blockDevice::DeviceInfo info;
        try {
            zbon::decode(deviceData[index], info);
        } catch (zbon::DecoderException &e) {
            std::cerr << "received malformed DeviceInfo field on class subscription" << std::endl;
            /* ignore malformed device */
            continue;
        }
        auto device = std::make_unique<Device>(std::move(devicePorts[index]),
                                               deviceIDs[index],
                                               info);
        DeviceList::instance.add(std::move(device));
    }
    std::cout << "StorageEngine startup complete, waiting for more block devices" << std::endl;
    zagtos::DefaultEventLoop();
}
