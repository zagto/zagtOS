#include <zagtos/IOPortRange.hpp>
extern "C" {
    #include <acpi.h>
}

static zagtos::IOPortRange fullRange(0, 0xffff);

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    assert(Width % 8 == 0 && Width <= 32);
    *Value = fullRange.read(Address, Width / 8);
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    assert(Width % 8 == 0 && Width <= 32);
    fullRange.write(Address, Width / 8, Value);
    return AE_OK;
}
