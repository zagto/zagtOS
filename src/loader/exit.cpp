#include <exit.hpp>
#include <Paging.hpp>
#include <log/Logger.hpp>

#define TEMPORY_STACK_SIZE 16 // in size_ts

void ExitToKernel(size_t entry, PageTable *newMasterPageTable, const struct BootInfo *bootInfo) {
    static size_t temporaryStack[TEMPORY_STACK_SIZE];
    cout << "Temporary stack at: " << reinterpret_cast<size_t>(&temporaryStack) << endl;

    ExitFinalize(entry,
                 newMasterPageTable,
                 bootInfo,
                 temporaryStack + TEMPORY_STACK_SIZE);
}
