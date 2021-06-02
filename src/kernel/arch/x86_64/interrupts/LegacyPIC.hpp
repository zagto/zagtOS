#pragma once
#include <common/common.hpp>

/*
 * This Class represents the legacy 8259 PIC.
 * It's only purpose is properly disabling it on startup.
 *
 * For the interrupt controller that is actually used in Zagtos, see LocalAPIC.
 */
class LegacyPIC
{
public:
    LegacyPIC();
    void handleSpuriousIRQ(size_t interruptNumber);
};
