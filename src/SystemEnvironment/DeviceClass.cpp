#include "DeviceClass.hpp"
#include <zagtos/protocols/ClassDevice.hpp>

std::vector<std::shared_ptr<DeviceClass>> DeviceClassRegistry;

void DeviceClass::handleEvent(const zagtos::Event &event) {
    if (event.messageType() == zagtos::classDevice::MSG_SUBSCRIBE) {
        std::cout << "Device class subscribe message" << std::endl;
        zagtos::RemotePort subscriptionPort;
        try {
            zbon::decode(event.messageData(), subscriptionPort);
        } catch(zbon::DecoderException &e) {
            std::cout << "Received malformed MSG_SUBSCRIBE message." << std::endl;
            return;
        }
        zbon::EncodedData result = zbon::encodeObject(instancePorts, instanceData);
        subscriptionPort.sendMessage(zagtos::classDevice::MSG_SUBSCRIBE_RESULT, std::move(result));
        subscriptions.push_back(std::make_unique<DeviceClassSubscription>(std::move(subscriptionPort)));
    } else {
        std::cout << "Got unknown message on class device subscribe port" << std::endl;
    }
}

void DeviceClass::addInstance(zagtos::RemotePort remotePort, zagtos::MessageData data) {
    zbon::EncodedData subscriberMessage = zbon::encodeObject(remotePort, nextInstanceID, data);
    for (auto &subscription: subscriptions) {
        subscription->remotePort.sendMessage(zagtos::classDevice::MSG_FOUND_DEVICE,
                                             subscriberMessage);
    }

    instancePorts.push_back(std::move(remotePort));
    instanceData.push_back(std::move(data));
    instanceIDs.push_back(nextInstanceID);
    nextInstanceID++;
}
