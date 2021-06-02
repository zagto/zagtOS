#include <interrupts/GlobalDescriptorTable.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>


GlobalDescriptorTable::GlobalDescriptorTable(Status &status) {
    if (!status) {
        return;
    }

    size_t gdtSize = FIRST_TSS_ENTRY + 2 * CurrentSystem.numProcessors; /* in uint64_ts */
    Result result = make_raw_array<uint64_t>(gdtSize);
    if (!result) {
        status = result.status();
        return;
    }
    gdt = *result;

    gdt[NULL_ENTRY] = 0;
    gdt[KERNEL_CODE_ENTRY] = FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE | FLAG_CODE | FLAG_LONGMODE;
    gdt[KERNEL_DATA_ENTRY] = FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE;
    gdt[USER_DATA_ENTRY] = FLAG_USER | FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE;
    gdt[USER_CODE_ENTRY] = FLAG_USER | FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE | FLAG_CODE | FLAG_LONGMODE;

    for (size_t processorID = 0; processorID < CurrentSystem.numProcessors; processorID++) {
    }

    gdtr.address = gdt;
    gdtr.size = gdtSize * sizeof(uint64_t) - 1;
}

GlobalDescriptorTable::~GlobalDescriptorTable() {
    if (gdt != nullptr) {
        delete gdt;
    }
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
    loadGlobalDescriptorTable(&gdtr);
}
