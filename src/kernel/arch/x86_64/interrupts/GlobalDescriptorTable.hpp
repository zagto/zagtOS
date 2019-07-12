#ifndef GLOBALDESCRIPTORTABLE_HPP
#define GLOBALDESCRIPTORTABLE_HPP

#include <common/common.hpp>
#include <interrupts/TaskStateSegment.hpp>

class GlobalDescriptorTable
{
private:
    static const u64 FLAG_PRESENT   {0x00800000000000};
    static const u64 FLAG_SEGMENT   {0x00100000000000};
    static const u64 FLAG_CODE      {0x00080000000000};
    static const u64 FLAG_WRITEABLE {0x00020000000000};
    static const u64 FLAG_TSS_BUSY  {0x00020000000000};
    static const u64 FLAG_USER      {0x00600000000000};
    static const u64 FLAG_TSS       {0x00090000000000};
    static const u64 FLAG_LONGMODE  {0x20000000000000};

    u64 nullEntry;
    u64 kernelCodeEntry;
    u64 kernelDataEntry;
    u64 userCodeEntry;
    u64 userDataEntry;
    u64 taskStateSegmentEntry[2];

public:
    GlobalDescriptorTable(TaskStateSegment *taskStateSegment);

    void resetTaskStateSegment();
};

#endif // GLOBALDESCRIPTORTABLE_HPP
