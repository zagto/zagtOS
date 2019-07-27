#ifndef GLOBALDESCRIPTORTABLE_HPP
#define GLOBALDESCRIPTORTABLE_HPP

#include <common/common.hpp>
#include <interrupts/TaskStateSegment.hpp>

class GlobalDescriptorTable
{
private:
    static const uint64_t FLAG_PRESENT   {0x00800000000000};
    static const uint64_t FLAG_SEGMENT   {0x00100000000000};
    static const uint64_t FLAG_CODE      {0x00080000000000};
    static const uint64_t FLAG_WRITEABLE {0x00020000000000};
    static const uint64_t FLAG_TSS_BUSY  {0x00020000000000};
    static const uint64_t FLAG_USER      {0x00600000000000};
    static const uint64_t FLAG_TSS       {0x00090000000000};
    static const uint64_t FLAG_LONGMODE  {0x20000000000000};

    uint64_t nullEntry;
    uint64_t kernelCodeEntry;
    uint64_t kernelDataEntry;
    uint64_t userCodeEntry;
    uint64_t userDataEntry;
    uint64_t taskStateSegmentEntry[2];

public:
    GlobalDescriptorTable(TaskStateSegment *taskStateSegment);

    void resetTaskStateSegment();
};

#endif // GLOBALDESCRIPTORTABLE_HPP
