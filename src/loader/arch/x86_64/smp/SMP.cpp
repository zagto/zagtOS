#include <smp/SMP.hpp>
#include <smp/LocalAPIC.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <EFI.hpp> // Stall
#include <Firmware.hpp>
#include <memory/PhysicalMemory.hpp> // SecondaryProcessorEntry
#include <exit.hpp>

static size_t startingID = 0;
static size_t startingHardwareID = 0;
static bool currentlyStarting = false;
static bool releaseToKernel = false;

size_t BootProcessorHardwareID;

extern "C" size_t *CurrentEntryStack;
size_t *CurrentEntryStack = nullptr;

static void wakeSecondaryProcessor(LocalAPIC &localAPIC, size_t hardwareID, size_t processorID) {
    startingID = processorID;
    startingID = hardwareID;
    CurrentEntryStack = temporaryStack[processorID] + TEMPORY_STACK_SIZE;
    __atomic_store_n(&currentlyStarting, true, __ATOMIC_SEQ_CST);

    localAPIC.sendInit(static_cast<uint32_t>(hardwareID));
    efi::SystemTable->BootServices->Stall(10 * 1000); // 10 ms
    localAPIC.sendStartup(static_cast<uint32_t>(hardwareID), SecondaryProcessorEntry);

    while (__atomic_load_n(&currentlyStarting, __ATOMIC_SEQ_CST)) {}
}

extern "C" void LoaderEntrySecondaryProcessor() {
    size_t id = startingID;
    size_t hardwareID = startingHardwareID;
    __atomic_store_n(&currentlyStarting, false, __ATOMIC_SEQ_CST);

    while (!__atomic_load_n(&releaseToKernel, __ATOMIC_SEQ_CST)) {}
    ExitToKernel(id, hardwareID);
}

void releaseSecondaryProcessorsToKernel() {
    __atomic_store_n(&releaseToKernel, true, __ATOMIC_SEQ_CST);
}

/* logic from: https://wiki.osdev.org/MADT */
static const uint8_t *findMADT() {
    const uint8_t *rootTable = reinterpret_cast<const uint8_t *>(GetFirmwareRoot().value());

    const uint32_t entryLength = (reinterpret_cast<const uint32_t *>(rootTable)[4]);
    const uint8_t *nextPointer = rootTable + 36;

    while (nextPointer < rootTable + entryLength) {
        const uint8_t *currentEntry;
        if (rootTable[0] == 'X') {
            currentEntry = reinterpret_cast<const uint8_t *>(*reinterpret_cast<const uint64_t *>(nextPointer));
            nextPointer += 8;
        } else {
            currentEntry = reinterpret_cast<const uint8_t *>(*reinterpret_cast<const uint32_t *>(nextPointer));
            nextPointer += 4;
        }

        if (currentEntry[0] == 'A' && currentEntry[1] == 'P' && currentEntry[2] == 'I' && currentEntry[3] == 'C') {
            return currentEntry;
        }
    }

    cout << "could not find MADT table" << endl;
    Panic();
}

static size_t findProcessors() {
    const uint8_t *madt = findMADT();

    cout << "Detecting secondary Processors..." << endl;

    uint32_t madtLength = reinterpret_cast<const uint32_t *>(madt)[1];
    char *pointer = (char *)madt + 0x2c;

    bool foundBootProcessor = false;

    struct SubtableHeader {
        uint8_t type;
        uint8_t length;
    };
    struct LocalAPICSubtable {
        SubtableHeader header;
        uint8_t processorID;
        uint8_t id;
        uint32_t flags;
    };
    static const uint32_t ACPI_MADT_ENABLED = 1;

    size_t numProcessors = 0;

    LocalAPIC localAPIC(readModelSpecificRegister(MSR::IA32_APIC_BASE));

    while (pointer < (char *)madt + madtLength) {
        const SubtableHeader *subTable = reinterpret_cast<SubtableHeader *>(pointer);
        switch (subTable->type) {
        case 0: /* local APIC */ {
            const LocalAPICSubtable *lapic = reinterpret_cast<const LocalAPICSubtable *>(subTable);
            if (lapic->flags & ACPI_MADT_ENABLED) {
                /* The first of these entries is the processor this code is running on, don't try
                 * start it */
                if (foundBootProcessor) {
                    cout << "Found secondary processor - Processor ID: "
                         << static_cast<size_t>(lapic->processorID) << ", APIC ID: "
                         << static_cast<size_t>(lapic->id) << endl;
                    wakeSecondaryProcessor(localAPIC, lapic->id, numProcessors);
                } else {
                    foundBootProcessor = true;
                    BootProcessorHardwareID = lapic->id;
                }
                numProcessors++;
            }
        }
        }
        pointer += subTable->length;
    }
    return numProcessors;
}

void setupSecondaryProcessorEntry(PageTable *masterPageTable) {
    size_t length = secondaryProcessorEntryCodeLength();
    size_t mptOffset = static_cast<size_t>(&SecondaryProcessorEntryMasterPageTable
                                           - &SecondaryProcessorEntryCode);
    size_t mptAddress = reinterpret_cast<size_t>(masterPageTable);
    size_t entryCodeAddress = SecondaryProcessorEntry.value();

    assert(mptAddress < 0x100000000);
    assert(length <= PAGE_SIZE);

    cout << "loading secondary processor entry code to " << entryCodeAddress << endl;
    cout << "boot MPT at " << mptAddress << endl;

    uint8_t *destination = reinterpret_cast<uint8_t *>(entryCodeAddress);
    memcpy(destination, &SecondaryProcessorEntryCode, length);

    /* put the address the entry code is loaded to at offset 2 (immediate operand of mov) */
    memcpy(destination + 2, &entryCodeAddress, sizeof(uint32_t));
    memcpy(destination + mptOffset, &mptAddress, sizeof(uint32_t));

    MapAddress(PagingContext::HANDOVER,
               SecondaryProcessorEntry.value(),
               SecondaryProcessorEntry,
               true,
               true,
               false,
               CacheType::CACHE_NORMAL_WRITE_BACK);
}

size_t secondaryProcessorEntryCodeLength() {
    return static_cast<size_t>(&SecondaryProcessorEntryCodeEnd
                               - &SecondaryProcessorEntryCode);
}

size_t startSecondaryProcessors() {
    setupSecondaryProcessorEntry(GetCurrentMasterPageTable());
    size_t numProcessors = findProcessors();
    FreePhysicalFrame(SecondaryProcessorEntry);
    return numProcessors;
}
