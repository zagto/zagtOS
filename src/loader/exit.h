#ifndef EXIT_H
#define EXIT_H

#include <paging.h>
#include <bootinfo.h>

struct __attribute__((packed)) GlobalDescriptorTableRecord {
    UINT16 size;
    UINT64 *pointer;
};

__attribute__((noreturn))
void ExitToKernel(UINTN entry,
                  PageTable *newMasterPageTable,
                  const struct BootInfo *bootInfo);

__attribute__((noreturn))
void ExitFinalize(UINTN entry,
                  PageTable *newMasterPageTable,
                  const struct BootInfo *bootInfo,
                  UINTN *temporaryStack);

#endif // EXIT_H
