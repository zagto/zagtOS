#ifndef __ZAGTOS_PORTIO_H
#define __ZAGTOS_PORTIO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* special accessors for x86 I/O port address space */
uint32_t ZoReadPort(uint16_t address, size_t length);
void ZoWritePort(uint16_t address, size_t length, uint32_t value);

#endif
