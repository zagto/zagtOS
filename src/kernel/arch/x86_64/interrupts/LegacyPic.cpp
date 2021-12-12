#include <interrupts/LegacyPic.hpp>
#include <portio.hpp>

using namespace portio;

static const uint16_t MASTER_PIC_COMMAND = 0x20;
static const uint16_t MASTER_PIC_DATA    = 0x21;
static const uint16_t SLAVE_PIC_COMMAND  = 0xa0;
static const uint16_t SLAVE_PIC_DATA     = 0xa1;

static const uint8_t ICW1_ICW4 = 0x01;
static const uint8_t ICW1_INIT = 0x10;
static const uint8_t ICW4_8086 = 0x01;

/*
 * PIC initialization. based on:
 * https://wiki.osdev.org/PIC
 */
LegacyPIC::LegacyPIC() {
    OutB(MASTER_PIC_COMMAND, ICW1_ICW4 | ICW1_INIT);
    waitIO();
    OutB(SLAVE_PIC_COMMAND, ICW1_ICW4 | ICW1_INIT);
    waitIO();

    /* map IRQs 0-7 to 0x19-0x20 (master) and 0x1a-0x2f (slave). These will not be used for anything
     * else, so spurious can still be handled. */
    OutB(MASTER_PIC_DATA, 0x19);
    waitIO();
    OutB(SLAVE_PIC_DATA, 0x28);
    waitIO();

    OutB(MASTER_PIC_DATA, 4);
    waitIO();
    OutB(SLAVE_PIC_DATA, 2);
    waitIO();

    OutB(MASTER_PIC_DATA, ICW4_8086);
    waitIO();
    OutB(SLAVE_PIC_DATA, ICW4_8086);
    waitIO();

    /* Fully set both masks - We don't want interrupts from the legacy PIC */
    OutB(MASTER_PIC_DATA, 0xff);
    OutB(SLAVE_PIC_DATA, 0xff);
}


void LegacyPIC::handleSpuriousIRQ(size_t interruptNumber) {
    /* 0x21 = spurious irq form slave -> EOI to master */
    if (interruptNumber == 0x21) {
        OutB(MASTER_PIC_COMMAND, 0x20); /* 0x20 = EOI */
    }
}
