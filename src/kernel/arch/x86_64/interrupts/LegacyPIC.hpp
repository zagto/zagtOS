#ifndef LEGACYPIC_HPP
#define LEGACYPIC_HPP

#include <common/common.hpp>

/*
 * This Class represents the legacy 8259 PIC.
 * It's only purpose is properly disabling it on startup.
 *
 * For the interrupt controller that is actually used in Zagtos, see LocalAPIC.
 */
class LegacyPIC
{
private:
    void initialize();

public:
    LegacyPIC(bool bootProcessor);
    void handleSpuriousIRQ(size_t interruptNumber);
};

#endif // LEGACYPIC_HPP
