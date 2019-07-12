#include <zagtos/acpi.h>
#include <syscall.h>

size_t zagtos_get_acpi_root(void) {
    return zagtos_syscall(SYS_GET_ACPI_ROOT);
}
