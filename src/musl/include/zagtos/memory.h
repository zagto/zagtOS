#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void *zagtos_map_physical_area(uint64_t address, size_t length);
void zagtos_unmap_physical_area(void *pointer, size_t length);
size_t zagtos_get_physical_address(void *virtual_address);

/* special accessors for x86 I/O port address space */
uint32_t zagtos_io_port_read(uint16_t address, size_t length);
void zagtos_io_port_write(uint16_t address, size_t length, uint32_t value);

#endif // MEMORY_H
