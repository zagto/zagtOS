#include <interrupts/GlobalDescriptorTable.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <system/Processor.hpp>


GlobalDescriptorTable::GlobalDescriptorTable(Status &status) :
    gdt(FIRST_TSS_ENTRY + 2 * CurrentSystem.numProcessors /* GDT size in uint64_ts */, status) {

    if (!status) {
        return;
    }

    gdt[NULL_ENTRY] = 0;
    gdt[KERNEL_CODE_ENTRY] = FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE | FLAG_CODE | FLAG_LONGMODE;
    gdt[KERNEL_DATA_ENTRY] = FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE;
    gdt[USER_DATA_ENTRY] = FLAG_USER | FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE;
    gdt[USER_CODE_ENTRY] = FLAG_USER | FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE | FLAG_CODE | FLAG_LONGMODE;

    /* TSS entries are initialized later once the Processor calls the setupTSS method */

    gdtr.address = gdt.data();
    gdtr.size = gdt.size() * sizeof(uint64_t) - 1;
}

void GlobalDescriptorTable::setupTSS(size_t processorID, TaskStateSegment *tss) {
    uint64_t tssAddress = reinterpret_cast<uint64_t>(tss);
    static const uint64_t tssSize = sizeof(TaskStateSegment);

    gdt[FIRST_TSS_ENTRY + processorID * 2] = FLAG_PRESENT
            | FLAG_TSS
            | tssSize
            | ((tssAddress & 0x00ffffff) << 16)
            | ((tssAddress & 0xff000000) << 32);
    gdt[FIRST_TSS_ENTRY + processorID * 2 + 1] = (tssAddress & 0xffffffff00000000) >> 32;
}

void GlobalDescriptorTable::resetTSS(size_t processorID) {
    gdt[FIRST_TSS_ENTRY + 2 * processorID] &= ~FLAG_TSS_BUSY;
}

void GlobalDescriptorTable::load() {
    /* GDT load resets GS Base, which is used for CurrentProcessor pointer */
    Processor *backup = CurrentProcessor();
    loadGlobalDescriptorTable(&gdtr);
    InitCurrentProcessorPointer(backup);
}
