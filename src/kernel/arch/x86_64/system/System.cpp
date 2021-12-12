#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <system/Processor.hpp>
#include <ACPI.hpp>


hos_v1::System *_HandOverSystem;

System::System() :
        CommonSystem(*_HandOverSystem),
        gdt(),
        ACPIRoot{_HandOverSystem->firmwareRoot} {
    /* TODO: support for Intel PCIDs could be added here */
    tlbContextsPerProcessor = 1;

}

void System::setupCurrentProcessor() noexcept {
    gdt.load();
    idt.load();
    loadTaskStateSegment(CurrentProcessor()->id);
    setupSyscalls();
}

void System::lateInitialization() {
    detectIOAPICs();
    detectIRQRedirection();
}

void System::setupSyscalls() noexcept {
    cout << "registering syscall entry " << reinterpret_cast<uint64_t>(&syscallEntry) << endl;
    writeModelSpecificRegister(MSR::LSTAR, reinterpret_cast<uint64_t>(&syscallEntry));
    writeModelSpecificRegister(MSR::STAR, (static_cast<uint64_t>(0x08) << 32) | (static_cast<uint64_t>(0x10) << 48));
    writeModelSpecificRegister(MSR::SFMASK, RegisterState::FLAG_INTERRUPTS | RegisterState::FLAG_USER_IOPL);
}

void System::detectIOAPICs() {
    /* finds IOAPICs using ACPI tables */
    const MADTTable *madt = findMADT(ACPIRoot);
    assert(madt != nullptr);
    SubtableWrapper<IOAPICSubtable> subtables(madt);

    for (size_t index = 0; index < subtables.count(); index++) {
        IOAPICSubtable subtable = subtables[index];
        ioApics.push_back(apic::IOAPIC(subtable.address, subtable.gsiBase));
        cout << "IOAPIC at " << subtable.address << ", GSI offset " << subtable.gsiBase
             << ", entries count " << ioApics[index].numEntries() << endl;
    }
}

void System::detectIRQRedirection() noexcept {
    /* if there are no redirection entries, an 1-1 mapping from IRQ to GSI is assumed */
    for (size_t index = 0; index < 0xff; index++) {
        irqToGSI[index] = index;
    }

    const MADTTable *madt = findMADT(ACPIRoot);
    SubtableWrapper<IOAPICSubtable> subtables(madt);
}

