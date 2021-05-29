#include <iostream>
#include <zagtos/syscall.h>
#include <zagtos/Messaging.hpp>
#include <zagtos/HAL.hpp>
#include <PCI.hpp>
extern "C" {
    #include <acpi.h>
}


int main() {
    std::cout << "Hello from ACPI" << std::endl;

    zagtos::RemotePort envPort = zagtos::decodeRunMessage<zagtos::RemotePort>(zagtos::MSG_START_HAL);

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

    /*result = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (result) {
        throw std::logic_error("AcpiEnableSubsystem failed: " + std::to_string(result));
    }
    std::cout << "ACPI Subsystem initialized" << std::endl;

    result = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (result) {
        throw std::logic_error("AcpiInitializeObjects failed: " + std::to_string(result));
    }
    std::cout << "ACPI Objects initialized" << std::endl;*/

    initPCIForOS(envPort);

    std::cout << "ACPI HAL initialized" << std::endl;

    envPort.sendMessage(zagtos::MSG_START_HAL_RESULT, zbon::encode(true));
    std::cout << "ACPI HAL EXIT" << std::endl;

}

