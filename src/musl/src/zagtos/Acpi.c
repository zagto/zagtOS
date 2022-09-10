#include <zagtos/Acpi.h>
#include <syscall.h>

size_t ZoGetAcpiRoot(void) {
    return zagtos_syscall(SYS_GET_ACPI_ROOT);
}
