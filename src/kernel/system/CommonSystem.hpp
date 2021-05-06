#pragma once

#include <common/common.hpp>
#include <vector>
#include <system/Processor.hpp>
#include <memory/FrameManagement.hpp>
#include <setup/HandOverState.hpp>
#include <common/globalregisters.hpp>
#include <time/Time.hpp>
#include <paging/PagingContext.hpp>
#include <processes/FutexManager.hpp>
#include <lib/Status.hpp>


class CommonSystem {
private:
    FutexFrameID nextFutexFrameID;
    uint64_t nextTLBTimestamp{1};

public:
    Time time;
    /* Memory and the PaginContext must be initialized before dynamic memory can be used */
    PagingContext kernelOnlyPagingContext;
    FutexManager futexManager;

    const size_t numProcessors;
    /* initialized by arch-specific constructor */
    size_t tlbContextsPerProcessor{0};

    CommonSystem(const hos_v1::System &handOver, Status &status);

    Status initProcessorsAndTLB();
    FutexFrameID getNewFutexFrameID();
    uint64_t getNextTLBTimetamp();
};
