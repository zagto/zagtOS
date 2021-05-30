#include <smp/SMP.hpp>
#include <smp/LocalAPIC.hpp>
#include <Time.hpp>
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
    CurrentEntryStack = &temporaryStack[processorID][TEMPORY_STACK_SIZE / 2];
    cout << "1 ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;


    __atomic_store_n(&currentlyStarting, true, __ATOMIC_SEQ_CST);

    localAPIC.sendInit(static_cast<uint32_t>(hardwareID));
    delayMilliseconds(10);
    localAPIC.sendStartup(static_cast<uint32_t>(hardwareID), SecondaryProcessorEntry);

    while (__atomic_load_n(&currentlyStarting, __ATOMIC_SEQ_CST)) {}


    cout << "3 ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;


}

extern "C" void LoaderEntrySecondaryProcessor() {
    size_t id = startingID;
    size_t hardwareID = startingHardwareID;

    cout << "2 ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;

    cout << "Processor " << id << " online" << endl;

    __atomic_store_n(&currentlyStarting, false, __ATOMIC_SEQ_CST);

    while (!__atomic_load_n(&releaseToKernel, __ATOMIC_SEQ_CST)) {}
    ExitToKernel(id, hardwareID);
}

void releaseSecondaryProcessorsToKernel() {
    __atomic_store_n(&releaseToKernel, true, __ATOMIC_SEQ_CST);
}

struct RSDPTable {
    char signature[8];
    uint8_t checksum;
    char oemID[6];
    uint8_t revision;
    uint32_t rsdtAddress;
    /* ACPI 2.0+ only: */
    uint32_t length;
    uint64_t xsdtAddress;
};

struct TableHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemID[6];
    char oemTableID[8];
    uint32_t oemRevision;
    char aslCompilderID[4];
    uint32_t alsCompilerRevision;
};

struct MADTTable : TableHeader {
    uint32_t localAPICAddress;
    uint32_t flags;
};

struct MADTSubtableHeader {
    uint8_t type;
    uint8_t length;
};

struct LocalAPICSubtable : MADTSubtableHeader {
    uint8_t processorID;
    uint8_t id;
    uint32_t flags;
};


static const MADTTable *findMADT(const TableHeader *rsdt) {
    size_t pointerSize;
    if (memcmp(rsdt->signature, "RSDT", 4) == 0) {
        pointerSize = 4;
    } else if (memcmp(rsdt->signature, "XSDT", 4) == 0) {
        pointerSize = 8;
    } else {
        cout << "Could not detect RSDT-like table type" << endl;
        Panic();
    }

    const uint8_t *pointerPointer = reinterpret_cast<const uint8_t *>(rsdt + 1);
    const TableHeader *pointer;

    while (pointerPointer < reinterpret_cast<const uint8_t *>(rsdt) + rsdt->length) {
        /* assumes litte-endian */
        memcpy(&pointer, pointerPointer, pointerSize);

        if (memcmp(pointer->signature, "APIC", 4) == 0) {
            return static_cast<const MADTTable *>(pointer);
        }

        pointerPointer += pointerSize;
    }

    cout << "Could not find MADT table." << endl;
    Panic();
}

static const MADTTable *findMADT() {

    const RSDPTable *rsdp = reinterpret_cast<const RSDPTable *>(GetFirmwareRoot().value());
    if (rsdp->revision == 0) {
        cout << "Found ACPI Version 1.0." << endl;
        return findMADT(reinterpret_cast<const TableHeader *>(rsdp->rsdtAddress));
    } else {
        cout << "Found ACPI Version 2.0+." << endl;
        return findMADT(reinterpret_cast<const TableHeader *>(rsdp->xsdtAddress));
    }
}

static size_t findProcessors() {
    cout << "Detecting Processors using ACPI..." << endl;

    const MADTTable *madt = findMADT();
    const uint8_t *pointer = reinterpret_cast<const uint8_t *>(madt + 1);
    bool foundBootProcessor = false;
    size_t numProcessors = 0;

    static const uint32_t ACPI_MADT_ENABLED = 1;
    static const uint8_t ACPI_MADT_TYPE_LOCAL_APIC = 0;

    LocalAPIC localAPIC(madt->localAPICAddress);
    cout << "Local APIC is at " << static_cast<size_t>(madt->localAPICAddress) << endl;

    while (pointer < (uint8_t *)madt + madt->length) {
        MADTSubtableHeader subTableHeader;
        memcpy(&subTableHeader, pointer, sizeof(MADTSubtableHeader));

        switch (subTableHeader.type) {
        case ACPI_MADT_TYPE_LOCAL_APIC: {
            LocalAPICSubtable lapic;
            memcpy(&lapic, pointer, sizeof(LocalAPICSubtable));

            if (lapic.flags & ACPI_MADT_ENABLED) {
                /* The first of these entries is the processor this code is running on, don't try
                 * start it */
                if (foundBootProcessor) {
                    cout << "Found secondary processor - ACPI Processor ID: "
                         << static_cast<size_t>(lapic.processorID) << ", our Processor ID: "
                         << static_cast<size_t>(numProcessors) << ", APIC ID: "
                         << static_cast<size_t>(lapic.id) << endl;

                    wakeSecondaryProcessor(localAPIC, lapic.id, numProcessors);
                } else {
                    cout << "Found boot processor - ACPI Processor ID: "
                         << static_cast<size_t>(lapic.processorID) << ", APIC ID: "
                         << static_cast<size_t>(lapic.id) << endl;
                    foundBootProcessor = true;
                    BootProcessorHardwareID = lapic.id;
                }
                numProcessors++;
            }
            break;
        }
        }
        pointer += subTableHeader.length;
    }
    return numProcessors;
}

void setupSecondaryProcessorEntry(PageTable *masterPageTable) {
    size_t length = secondaryProcessorEntryCodeLength();
    size_t mptOffset = static_cast<size_t>(&SecondaryProcessorEntryMasterPageTable
                                           - &SecondaryProcessorEntryCode);
    size_t targetAddrOffset = static_cast<size_t>(&SecondaryProcessorEntryTarget
                                           - &SecondaryProcessorEntryCode);
    size_t stackPtrOffset = static_cast<size_t>(&SecondaryProcessorEntryStackPointerPointer
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
    void *functionPointer = reinterpret_cast<void *>(&LoaderEntrySecondaryProcessor);
    memcpy(destination + targetAddrOffset, &functionPointer, sizeof(uint64_t));
    size_t **stackPointerPointer = &CurrentEntryStack;
    memcpy(destination + stackPtrOffset, &stackPointerPointer, sizeof(uint64_t));


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
    cout << "a ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;


    setupSecondaryProcessorEntry(HandOverMasterPageTable);
    cout << "b ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;


    size_t numProcessors = findProcessors();
    cout << "c ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;


    FreePhysicalFrame(SecondaryProcessorEntry);
    cout << "d ------ Handover Master Page Table is at: "
         << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;

    return numProcessors;
}
