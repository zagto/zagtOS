#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

void zagtos_register_interrupt(uint64_t irq);
void zagtos_unregister_interrupt(uint64_t irq);


#endif // INTERRUPT_H
