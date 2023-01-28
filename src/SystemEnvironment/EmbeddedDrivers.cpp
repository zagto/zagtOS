#include "EmbeddedDrivers.hpp"
#include "Driver.hpp"
#include "DeviceClass.hpp"
#include <zagtos/protocols/StorageEngine.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/protocols/Driver.hpp>
#include <zagtos/protocols/ClassDevice.hpp>
#include <zagtos/protocols/BlockDevice.hpp>

EXTERNAL_BINARY(ACPIHAL)
EXTERNAL_BINARY(DeviceTreeHAL)
EXTERNAL_BINARY(PCIController)
EXTERNAL_BINARY(AHCIDriver)
EXTERNAL_BINARY(PS2Controller)
EXTERNAL_BINARY(StorageEngine)

/* passed to StorageEngine */
std::shared_ptr<DeviceClass> blockDeviceClass;

void RegisterEmbeddedDrivers() {
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        ACPIHAL,
        {},
        {},
        {zagtos::driver::CONTROLLER_TYPE_ROOT}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        DeviceTreeHAL,
        {},
        {},
        {zagtos::driver::CONTROLLER_TYPE_ROOT}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        PCIController,
        {{zagtos::driver::CONTROLLER_TYPE_ROOT, zagtos::driver::RootDevice::PCI_CONTROLLER, DeviceMatch::EXACT_MASK}},
        {},
        {zagtos::driver::CONTROLLER_TYPE_PCI}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        PS2Controller,
        {{zagtos::driver::CONTROLLER_TYPE_ROOT, zagtos::driver::RootDevice::PS2_CONTROLLER, DeviceMatch::EXACT_MASK}},
        {},
        {zagtos::driver::CONTROLLER_TYPE_PS2}}));
    DriverRegistry.push_back(std::make_shared<Driver>(Driver{
        AHCIDriver,
        {{zagtos::driver::CONTROLLER_TYPE_PCI, 0x0106'0000'0000'0000, 0xffff'0000'0000'0000}},
        {zagtos::blockDevice::DEVICE_CLASS},
        {}}));

    blockDeviceClass = std::make_shared<DeviceClass>(zagtos::blockDevice::DEVICE_CLASS);
    DeviceClassRegistry.push_back(blockDeviceClass);
}

void StartStorageEngine() {
    zagtos::MessageData runMessage = zbon::encode(blockDeviceClass->port);
    environmentSpawn(StorageEngine,
                     zagtos::Priority::BACKGROUND,
                     zagtos::storageEngine::MSG_START,
                     runMessage);
}

