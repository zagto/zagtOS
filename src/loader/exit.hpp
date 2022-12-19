#pragma once
#include <Paging.hpp>
#include <setup/HandOverState.hpp>

#define TEMPORY_STACK_SIZE 128 // in size_ts

extern size_t temporaryStack[TEMPORY_STACK_SIZE][512];
extern size_t KernelEntryAddress;
extern hos_v1::System *BootInfo;

__attribute__((noreturn))
void ExitToKernel(size_t processorID, size_t hardwareID, hos_v1::PagingContext *pagingContext);

extern "C" {
__attribute__((noreturn))
void ExitFinalize(size_t entry,
                  hos_v1::PagingContext *handoverPagingContext,
                  const hos_v1::System *bootInfo,
                  size_t *temporaryStack,
                  size_t processorID,
                  size_t hardwareID);
}
