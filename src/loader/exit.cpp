#include <exit.hpp>
#include <Paging.hpp>
#include <iostream>

#define TEMPORY_STACK_SIZE 128 // in size_ts

alignas (__uint128_t) size_t temporaryStack[TEMPORY_STACK_SIZE][512];
size_t KernelEntryAddress = 0;
hos_v1::System *BootInfo = nullptr;

void ExitToKernel(size_t processorID, size_t hardwareID, hos_v1::PagingContext *pagingContext) {
    assert(processorID < 512);

    ExitFinalize(KernelEntryAddress,
                 pagingContext,
                 BootInfo,
                 temporaryStack[processorID] + TEMPORY_STACK_SIZE,
                 processorID,
                 hardwareID);
}
