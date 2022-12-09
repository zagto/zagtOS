#pragma once

#include <common/common.hpp>
#include <vector>
#include <memory/FrameManagement.hpp>
#include <setup/HandOverState.hpp>
#include <time/Time.hpp>
#include <paging/PagingContext.hpp>
#include <processes/FutexManager.hpp>


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
    PhysicalAddress firmwareRoot;

    CommonSystem(const hos_v1::System &handOver);

    void initProcessorsAndTLB();
    FutexFrameID getNewFutexFrameID() noexcept;
    uint64_t getNextTLBTimetamp() noexcept;
};
