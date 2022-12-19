#include <IOAPIC.hpp>
#include <iostream>
#include <zagtos/Messaging.hpp>
#include <vector>
extern "C" {
    #include <acpi.h>
}

using namespace zagtos;


void initIoApic(zagtos::RemotePort &) {

    ACPI_OBJECT object {
        .Integer = {
            .Type = ACPI_TYPE_INTEGER,
            .Value = 1, /* 1 = APIC mode */
        }
    };
    ACPI_OBJECT_LIST parameters {
        .Count = 1,
        .Pointer = &object
    };

    ACPI_STATUS status = AcpiEvaluateObject(nullptr,
                                            const_cast<ACPI_STRING>("\\_PIC"),
                                            &parameters,
                                            nullptr);
    if (status != AE_OK) {
        std::cerr << "warning: _PIC method call failed: " << status << std::endl;
    }
}
