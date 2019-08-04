#define _GNU_SOURCE 1
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <syscall.h>
#include <memory.h>
#include <limits.h>
#include <sys/mman.h>

void *zagtos_map_physical_area(uint64_t physical_address, size_t length) {
    size_t offset = physical_address % PAGE_SIZE;
    length += offset;

    return (uint8_t *)mmap(0,
                           length,
                           PROT_READ|PROT_WRITE,
                           MAP_SHARED|MAP_PHYSICAL,
                           -1,
                           (off_t)(physical_address - offset)) + offset;
}

void zagtos_unmap_physical_area(void *pointer, size_t length) {
    size_t offset = ((size_t)pointer) % PAGE_SIZE;
    length += offset;

    munmap((char *)pointer - offset, length);
}

size_t zagtos_get_physical_address(void *virtual_address) {
    return zagtos_syscall(SYS_GET_PHYSICAL_ADDRESS, virtual_address);
}

uint32_t zagtos_io_port_read(uint16_t address, size_t length) {
    assert(length == 1 || length == 2 || length == 4);
    return (uint32_t)zagtos_syscall(SYS_IO_PORT_READ, address, length);
}

void zagtos_io_port_write(uint16_t address, size_t length, uint32_t value) {
    assert(length == 1 || length == 2 || length == 4);
    zagtos_syscall(SYS_IO_PORT_WRITE, address, length, (uint64_t)value);
}
