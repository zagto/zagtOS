#pragma once
#include <Paging.hpp>

__attribute__((noreturn))
void ExitToKernel(size_t entry,
                  PageTable *newMasterPageTable,
                  const struct BootInfo *bootInfo);

__attribute__((noreturn))
void ExitFinalize(size_t entry,
                  PageTable *newMasterPageTable,
                  const struct BootInfo *bootInfo,
                  size_t *temporaryStack);
