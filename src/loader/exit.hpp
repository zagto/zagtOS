#pragma once
#include <Paging.hpp>
#include <setup/HandOverState.hpp>

__attribute__((noreturn))
void ExitToKernel(size_t entry,
                  PageTable *newMasterPageTable,
                  const hos_v1::System *bootInfo);

extern "C" {
__attribute__((noreturn))
void ExitFinalize(size_t entry,
                  PageTable *newMasterPageTable,
                  const hos_v1::System *bootInfo,
                  size_t *temporaryStack);
}
