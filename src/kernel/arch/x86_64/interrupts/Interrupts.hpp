#pragma once

#include <common/common.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/Record.hpp>
#include <interrupts/RegisterState.hpp>
#include <interrupts/LocalAPIC.hpp>

static const size_t PIC1_SPURIOUS_IRQ = 0x20;
static const size_t PIC2_SPURIOUS_IRQ = 0x21;

extern "C" void basicEnableInterrupts();
extern "C" void basicDisableInterrupts();

[[noreturn]] void _handleInterrupt(RegisterState* registerState);
void dealWithException(Status status);

extern InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;
