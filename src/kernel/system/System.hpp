#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <common/common.hpp>
#include <lib/vector.hpp>
#include <system/Processor.hpp>
#include <memory/Memory.hpp>
#include <setup/BootInfo.hpp>
#include <common/globalregisters.hpp>


class System {
public:
    Memory memory;
    /* Memory and the MasterPageTable must be initialized before dynamic memory can be used */
    MasterPageTable *kernelOnlyMasterPageTable;
    vector<Processor *> processors;

    System(BootInfo *bootInfo) :
        memory(bootInfo),
        kernelOnlyMasterPageTable{bootInfo->masterPageTable.identityMapped().asPointer<MasterPageTable>()},
        processors() {}

    void addBootProcessor();
};

// make sure System fits in the fixed area assigned for it
static_assert(sizeof(System) < PAGE_SIZE - 6 * 8 - 22 * 8);

extern System CurrentSystem;

#endif // SYSTEM_HPP
