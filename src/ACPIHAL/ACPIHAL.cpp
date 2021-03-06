#include <iostream>
#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Hal.hpp>
#include <PCI.hpp>
#include <IOAPIC.hpp>
#include <LegacyDevices.hpp>
extern "C" {
    #include <acpi.h>
}


int main() {
    std::cout << "Hello from ACPI" << std::endl;

    zagtos::RemotePort envPort = zagtos::decodeRunMessage<zagtos::RemotePort>(
                zagtos::hal::MSG_START);

    std::cout << "Decoded handle" << std::endl;


    if (AcpiInitializeSubsystem()) {
        throw std::logic_error("AcpiInitializeSubsystem failed");
    }
    std::cout << "ACPI Subsystem initialized" << std::endl;

    if (AcpiInitializeTables(nullptr, 10, true)) {
        throw std::logic_error("AcpiInitializeTables failed");
    }
    std::cout << "ACPI Tables initialized" << std::endl;


    initPCIForACPI();

    ACPI_STATUS result = AcpiLoadTables();
    if (result) {
        throw std::logic_error("AcpiLoadTables failed: " + std::to_string(result));
        return 1;
    }
    std::cout << "ACPI Tables initialized" << std::endl;

    result = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (result) {
        throw std::logic_error("AcpiEnableSubsystem failed: " + std::to_string(result));
    }
    std::cout << "ACPI Subsystem initialized" << std::endl;

    result = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (result) {
        throw std::logic_error("AcpiInitializeObjects failed: " + std::to_string(result));
    }
    std::cout << "ACPI Objects initialized" << std::endl;

    initIoApic(envPort);
    initPCIForOS(envPort);
    initLegacyDevices(envPort);

    std::cout << "ACPI HAL initialized" << std::endl;

    envPort.sendMessage(zagtos::hal::MSG_START_RESULT, zbon::encode(true));
    std::cout << "ACPI HAL EXIT" << std::endl;

    zagtos::Port dummy;
    dummy.receiveMessage();
}

