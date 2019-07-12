#include <interrupts/GlobalDescriptorTable.hpp>


GlobalDescriptorTable::GlobalDescriptorTable(TaskStateSegment *taskStateSegment) {
    nullEntry = 0;
    kernelCodeEntry = FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE | FLAG_CODE | FLAG_LONGMODE;
    kernelDataEntry = FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE;
    userCodeEntry =
            FLAG_USER | FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE | FLAG_CODE | FLAG_LONGMODE;
    userDataEntry = FLAG_USER | FLAG_PRESENT | FLAG_SEGMENT | FLAG_WRITEABLE;

    u64 taskStateSegmentAddress = reinterpret_cast<u64>(taskStateSegment);
    u64 taskStateSegmentSize = sizeof(TaskStateSegment);
    taskStateSegmentEntry[0] = FLAG_PRESENT
            | FLAG_TSS
            | taskStateSegmentSize
            | ((taskStateSegmentAddress & 0x00ffffff) << 16)
            | ((taskStateSegmentAddress & 0xff000000) << 32);
    taskStateSegmentEntry[1] = (taskStateSegmentAddress & 0xffffffff00000000) >> 32;
}


void GlobalDescriptorTable::resetTaskStateSegment() {
    taskStateSegmentEntry[0] &= ~FLAG_TSS_BUSY;
}
