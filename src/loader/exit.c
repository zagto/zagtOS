#include <efi.h>
#include <exit.h>


#define TEMPORY_STACK_SIZE 16 // in UINTNs

void ExitToKernel(UINTN entry, PageTable *newMasterPageTable, const struct BootInfo *bootInfo) {
    static UINTN temporaryStack[TEMPORY_STACK_SIZE];
    Log("Temporary stack at: ");
    LogUINTN((UINTN)&temporaryStack);
    Log("\n");

    ExitFinalize(entry,
                 newMasterPageTable,
                 bootInfo,
                 temporaryStack + TEMPORY_STACK_SIZE);
}
