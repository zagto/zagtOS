#include <smp/SMP.hpp>
#include <smp/LocalAPIC.hpp>
#include <Time.hpp>
#include <Firmware.hpp>
#include <memory/PhysicalMemory.hpp> // SecondaryProcessorEntry
#include <exit.hpp>
#include <ACPI.hpp>
#include <iostream>

static size_t startingID = 0;
static size_t startingHardwareID = 0;
static bool currentlyStarting = false;
static bool releaseToKernel = false;
static hos_v1::PagingContext handOverPagingContext;

size_t BootProcessorHardwareID;

extern "C" size_t *CurrentEntryStack;
size_t *CurrentEntryStack = nullptr;

static void wakeSecondaryProcessor(LocalAPIC &localAPIC, size_t hardwareID, size_t processorID) {
    startingID = processorID;
    startingHardwareID = hardwareID;
    CurrentEntryStack = &temporaryStack[processorID][TEMPORY_STACK_SIZE / 2];


    __atomic_store_n(&currentlyStarting, true, __ATOMIC_SEQ_CST);

    localAPIC.sendInit(static_cast<uint32_t>(hardwareID));
    delayMilliseconds(10);
    localAPIC.sendStartup(static_cast<uint32_t>(hardwareID), SecondaryProcessorEntry);

    while (__atomic_load_n(&currentlyStarting, __ATOMIC_SEQ_CST)) {}
}

extern "C" void LoaderEntrySecondaryProcessor() {
    size_t id = startingID;
    size_t hardwareID = startingHardwareID;

    cout << "Processor " << id << " hardwareID " << hardwareID << " online" << endl;

    __atomic_store_n(&currentlyStarting, false, __ATOMIC_SEQ_CST);

    while (!__atomic_load_n(&releaseToKernel, __ATOMIC_SEQ_CST)) {}
    ExitToKernel(id, hardwareID, &handOverPagingContext);
}

void releaseSecondaryProcessorsToKernel() {
    __atomic_store_n(&releaseToKernel, true, __ATOMIC_SEQ_CST);
}

static size_t findProcessors() {
    cout << "Detecting Processors using ACPI..." << endl;

    const MADTTable *madt = findMADT(GetFirmwareInfo().rootAddress);

    bool foundBootProcessor = false;
    size_t numProcessors = 0;

    LocalAPIC localAPIC(madt->localAPICAddress);
    cout << "Local APIC is at " << static_cast<size_t>(madt->localAPICAddress) << endl;

    SubtableWrapper<LocalAPICSubtable> subtables(madt);

    for (size_t index = 0; index < subtables.count(); index++) {
        LocalAPICSubtable lapic = subtables[index];

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
    }

    return numProcessors;
}



void setupSecondaryProcessorEntry() {
    size_t length = secondaryProcessorEntryCodeLength();
    size_t mptOffset = static_cast<size_t>(&SecondaryProcessorEntryMasterPageTable
                                           - &SecondaryProcessorEntryCode);
    size_t targetAddrOffset = static_cast<size_t>(&SecondaryProcessorEntryTarget
                                           - &SecondaryProcessorEntryCode);
    size_t stackPtrOffset = static_cast<size_t>(&SecondaryProcessorEntryStackPointerPointer
                                           - &SecondaryProcessorEntryCode);
    size_t mptAddress = handOverPagingContext.root.value();
    size_t entryCodeAddress = SecondaryProcessorEntry.value();

    assert(mptAddress < 0x100000000);
    assert(length <= PAGE_SIZE);

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
    handOverPagingContext = GetPagingContext();
    setupSecondaryProcessorEntry();
    size_t numProcessors = findProcessors();
    FreePhysicalFrame(SecondaryProcessorEntry);
    return numProcessors;
}
