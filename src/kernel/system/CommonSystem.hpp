#ifndef COMMONSYSTEM_HPP
#define COMMONSYSTEM_HPP

#include <common/common.hpp>
#include <lib/vector.hpp>
#include <system/Processor.hpp>
#include <memory/Memory.hpp>
#include <setup/BootInfo.hpp>
#include <common/globalregisters.hpp>
#include <time/Time.hpp>
#include <paging/PagingContext.hpp>


class CommonSystem {
public:
    Memory memory;
    Time time;
    /* Memory and the MasterPageTable must be initialized before dynamic memory can be used */
    PagingContext kernelOnlyPagingContext;
    vector<Processor *> processors;
    Lock processorsLock;

    CommonSystem(BootInfo *bootInfo) :
        memory(bootInfo),
        kernelOnlyPagingContext(nullptr, bootInfo->masterPageTable),
        processors() {}

    void addBootProcessor();
};

#endif // COMMONSYSTEM_HPP
