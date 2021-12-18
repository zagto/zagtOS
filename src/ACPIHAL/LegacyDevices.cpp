#include <LegacyDevices.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/Interrupt.hpp>
#include <zagtos/IOPortRange.hpp>
#include <zagtos/protocols/Hal.hpp>
#include <array>
extern "C" {
#include <acpi.h>
}

using namespace zagtos;

struct IRQRedirect {
    uint32_t gsi;
    TriggerMode triggerMode;
    Polarity polarity;
};

static std::array<IRQRedirect, 256> redirects;

ACPI_TABLE_FADT *fadt;
ACPI_TABLE_MADT *madt;

static void getTables() {
    ACPI_STATUS status = AcpiGetTable(const_cast<ACPI_STRING>(ACPI_SIG_MADT),
                                      0,
                                      (ACPI_TABLE_HEADER **)&madt);
    if (status != AE_OK) {
        throw std::logic_error("could not get MADR table");
    }

    status = AcpiGetTable(const_cast<ACPI_STRING>(ACPI_SIG_FADT),
                          0,
                          (ACPI_TABLE_HEADER **)&fadt);
    if (status != AE_OK) {
        throw std::logic_error("could not get FADT table");
    }
}

static void initIRQRedirects() {
    /* initialize IRQ redirects to defaults, which apply if there is no redirect entry in ACPI */
    for (uint32_t irq = 0; irq < 256; irq++) {
        redirects[irq] = {
            .gsi = irq,
            .triggerMode = TriggerMode::Edge,
            .polarity = Polarity::ActiveHigh,
        };
    }

    uint8_t *subtablePointer = reinterpret_cast<uint8_t *>(madt) + sizeof(ACPI_TABLE_MADT);
    uint8_t *end = reinterpret_cast<uint8_t *>(madt) + madt->Header.Length;

    while (subtablePointer < end) {
        auto subtable = reinterpret_cast<ACPI_MADT_INTERRUPT_OVERRIDE *>(subtablePointer);
        if (subtable->Header.Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE) {
            uint16_t acpiPolarity = subtable->IntiFlags & ACPI_MADT_POLARITY_MASK;
            uint16_t acpiTriggerMode = subtable->IntiFlags & ACPI_MADT_TRIGGER_MASK;
            Polarity polarity;
            TriggerMode triggerMode;

            switch (acpiPolarity) {
            case ACPI_MADT_POLARITY_ACTIVE_HIGH:
                polarity = Polarity::ActiveHigh;
                break;
            case ACPI_MADT_POLARITY_ACTIVE_LOW:
                polarity = Polarity::ActiveLow;
                break;
            /* behavoir found in sys/x86/acpica/madt.c in FreeBSD */
            default:
                if (subtable->SourceIrq == fadt->SciInterrupt) {
                    polarity = Polarity::ActiveLow;
                } else {
                    polarity = Polarity::ActiveHigh;
                }
            }

            switch (acpiTriggerMode) {
            case ACPI_MADT_TRIGGER_EDGE:
                triggerMode = TriggerMode::Edge;
                break;
            case ACPI_MADT_TRIGGER_LEVEL:
                triggerMode = TriggerMode::Level;
                break;
            /* behavoir found in sys/x86/acpica/madt.c in FreeBSD */
            default:
                if (subtable->SourceIrq == fadt->SciInterrupt) {
                    triggerMode = TriggerMode::Level;
                } else {
                    triggerMode = TriggerMode::Edge;
                }
            }

            redirects[subtable->SourceIrq] = {
                .gsi = subtable->GlobalIrq,
                .triggerMode = triggerMode,
                .polarity = polarity,
            };
        }
        subtablePointer += subtable->Header.Length;
    }
}

static Interrupt createLegacyInterrupt(uint8_t irq) {
    return Interrupt(redirects[irq].gsi, redirects[irq].triggerMode, redirects[irq].polarity);
}

static void initPS2Controller(RemotePort &environmentPort) {
    /*if (!(fadt->BootFlags & ACPI_FADT_8042)) {
        std::cout << "No 8042 PS/2 controller exists" << std::endl;
        return;
    }*/

    Interrupt int1 = createLegacyInterrupt(1);
    Interrupt int12 = createLegacyInterrupt(12);

    zagtos::IOPortRange ports(0x60, 0x64);

    std::cout << "Found PS/2 controller" << std::endl;
    /* PS/2 Controller */
    environmentPort.sendMessage(
            hal::MSG_FOUND_CONTROLLER,
            zbon::encodeObject(
                hal::CONTROLLER_TYPE_PS2,
                std::make_tuple<Interrupt, Interrupt, IOPortRange>(
                        std::move(int1),
                        std::move(int12),
                        std::move(ports))));
}

void initLegacyDevices(RemotePort &environmentPort) {
    getTables();
    initIRQRedirects();
    initPS2Controller(environmentPort);
}
