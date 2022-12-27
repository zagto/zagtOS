#include "ClassDevice.hpp"
#include "DeviceClass.hpp"

ClassDevice::ClassDevice(std::shared_ptr<DeviceClass> deviceClass_,
                         zagtos::RemotePort consumerPort) :
    deviceClass{std::move(deviceClass_)},
    consumerPort{std::move(consumerPort)} {

    deviceClass->instances.insert(this);
}

ClassDevice::~ClassDevice() {
    deviceClass->instances.erase(this);
}
