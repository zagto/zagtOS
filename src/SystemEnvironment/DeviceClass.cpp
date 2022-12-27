#include "DeviceClass.hpp"
#include "ClassDevice.hpp"
#include <zagtos/protocols/ClassDevice.hpp>

std::vector<std::shared_ptr<DeviceClass>> DeviceClassRegistry;

void DeviceClass::handleMessage(const zagtos::Event &event) {
    if (event.messageType() == zagtos::classDevice::MSG_SUBSCRIBE) {
        std::cout << "Device class subscribe message" << std::endl;
        zagtos::RemotePort subscriptionPort;
        try {
            zbon::decode(event.messageData(), subscriptionPort);
        } catch(zbon::DecoderException &e) {
            std::cout << "Received malformed MSG_SUBSCRIBE message." << std::endl;
            return;
        }
        std::vector<const zagtos::RemotePort *> instancePorts;
        instancePorts.reserve(instances.size());
        for (const ClassDevice *device: instances) {
            instancePorts.push_back(&device->consumerPort);
        }
        zbon::EncodedData result = zbon::encode(instancePorts);
        subscriptionPort.sendMessage(zagtos::classDevice::MSG_SUBSCRIBE_RESULT, std::move(result));
        consumers.push_back(std::move(subscriptionPort));
    } else {
        std::cout << "Got unknown message on class device subscribe port" << std::endl;
    }
}
