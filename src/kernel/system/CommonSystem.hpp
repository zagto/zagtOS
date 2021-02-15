#pragma once

#include <common/common.hpp>
#include <vector>
#include <system/Processor.hpp>
#include <memory/Memory.hpp>
#include <setup/HandOverState.hpp>
#include <common/globalregisters.hpp>
#include <time/Time.hpp>
#include <paging/PagingContext.hpp>
#include <processes/FutexManager.hpp>
#include <lib/Status.hpp>


class CommonSystem {
public:
    Memory memory;
    Time time;
    /* Memory and the PaginContext must be initialized before dynamic memory can be used */
    PagingContext kernelOnlyPagingContext;
    vector<Processor *> processors;
    mutex processorsLock;
    FutexManager futexManager;

    CommonSystem(const hos_v1::System &handOver, Status &status) :
        memory(handOver),
        kernelOnlyPagingContext(nullptr, handOver.handOverPagingContext),
        processors(),
        futexManager(status) {}

    Status addBootProcessor();
};
