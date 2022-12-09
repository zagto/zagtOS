#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <system/Processor.hpp>
#include <processes/Process.hpp>
#include <ACPI.hpp>


hos_v1::System *_HandOverSystem;

System::System() :
        CommonSystem(*_HandOverSystem),
        gdt() {
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
}

void System::setupSyscalls() noexcept {
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

apic::IOAPIC &System::IOAPICForGSI(uint32_t gsi) {
    for (size_t index = 0; index < ioApics.size(); index++) {
        if (ioApics[index].isForGSI(gsi)) {
            return ioApics[index];
        }
    }
    cout << "Tried to setup a GSI that cannout be handled by out local APICs" << endl;
    /* TODO: a way for user space to recover from here may be useful */
    throw BadUserSpace(CurrentProcess());
}

void System::bindInterrupt(BoundInterrupt &boundInterrupt) {
    if (boundInterrupt.type == InterruptType::X86_GSI) {
        IOAPICForGSI(boundInterrupt.typeData).bindInterrutpt(boundInterrupt);
    }
}

void System::unbindInterrupt(BoundInterrupt &boundInterrupt) {
    if (boundInterrupt.type == InterruptType::X86_GSI) {
        IOAPICForGSI(boundInterrupt.typeData).unbindInterrutpt(boundInterrupt);
    }
}

void System::interruptFullyProcessed(BoundInterrupt &boundInterrupt) {
    if (boundInterrupt.type == InterruptType::X86_GSI
            && boundInterrupt.triggerMode.isLevel()) {
        IOAPICForGSI(boundInterrupt.typeData).endOfLevelInterrupt();
    }
}
