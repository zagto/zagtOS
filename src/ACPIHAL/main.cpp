#include <iostream>
#include <zagtos/syscall.h>
extern "C" {
    #include <acpi.h>
    #include <findProcessors.h>
}


int main() {
    std::cout << "Hello from ACPI" << std::endl;

    if (AcpiInitializeSubsystem()) {
        std::cout << "AcpiInitializeSubsystem failed" << std::endl;
        return 1;
    }
    std::cout << "ACPI Subsystem initialized" << std::endl;

    if (AcpiInitializeTables(nullptr, 10, true)) {
        std::cout << "AcpiInitializeTables failed" << std::endl;
        return 1;
    }
    std::cout << "ACPI Tables initialized" << std::endl;

    ACPI_TABLE_MCFG *mcfg;
    ACPI_STATUS result = AcpiGetTable(const_cast<ACPI_STRING>(ACPI_SIG_MCFG),
                                      0,
                                      reinterpret_cast<ACPI_TABLE_HEADER **>(&mcfg));
    if (result == AE_NOT_FOUND) {
        std::cout << "MCFG table does not exist" << std::endl;
        mcfg = nullptr;
    } else if (result) {
        std::cout << "Getting MCFG table failed: " << result << std::endl;
        return 1;
    }

    findProcessors();

    std::cout << "ACPI HAL initialized" << std::endl;

    /*
     * TODO
    result = AcpiLoadTables();
    if (result) {
        std::cout << "AcpiLoadTables failed: %u\n", result);
        return 1;
    }
    std::cout << "ACPI Tables initialized" << std::endl;

    result = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (result) {
        std::cout << "AcpiEnableSubsystem failed: %u\n", result);
        return 1;
    }
    std::cout << "ACPI Subsystem initialized" << std::endl;

    result = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (result) {
        std::cout << "AcpiInitializeObjects failed: %u\n", result);
        return 1;
    }
    std::cout << "ACPI Objects initialized" << std::endl;
    */

}

