#pragma once

#include <common/common.hpp>
#include <interrupts/TaskStateSegment.hpp>
#include <vector>

class GlobalDescriptorTable
{
private:
    struct __attribute__((__packed__)) GDTRStruct {
        uint16_t size;
        uint64_t *address;
    };

    static const uint64_t FLAG_PRESENT   {0x00800000000000};
    static const uint64_t FLAG_SEGMENT   {0x00100000000000};
    static const uint64_t FLAG_CODE      {0x00080000000000};
    static const uint64_t FLAG_WRITEABLE {0x00020000000000};
    static const uint64_t FLAG_TSS_BUSY  {0x00020000000000};
    static const uint64_t FLAG_USER      {0x00600000000000};
    static const uint64_t FLAG_TSS       {0x00090000000000};
    static const uint64_t FLAG_LONGMODE  {0x20000000000000};

    static const size_t NULL_ENTRY = 0;
    static const size_t KERNEL_CODE_ENTRY = 1;
    static const size_t KERNEL_DATA_ENTRY = 2;
    static const size_t USER_DATA_ENTRY = 3;
    static const size_t USER_CODE_ENTRY = 4;
    static const size_t FIRST_TSS_ENTRY = 5;

    GDTRStruct gdtr;
    vector<uint64_t> gdt;

public:
    GlobalDescriptorTable();

    void setupTSS(size_t processorID, TaskStateSegment *tss);
    void resetTSS(size_t processorID);
    void load();
};
