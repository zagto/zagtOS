#include <exit.hpp>
#include <Paging.hpp>
#include <log/Logger.hpp>

#define TEMPORY_STACK_SIZE 128 // in size_ts

void ExitToKernel(size_t entry, PageTable *newMasterPageTable, const hos_v1::System *bootInfo) {
    static size_t temporaryStack[TEMPORY_STACK_SIZE];
    cout << "Temporary stack at: " << reinterpret_cast<size_t>(&temporaryStack) << endl;
    cout << "kernel entry at: " << entry << endl;
    cout << "Handover master page table at: " << reinterpret_cast<size_t>(newMasterPageTable) << endl;
    cout << "Handover data at: " << reinterpret_cast<size_t>(bootInfo) << endl;

    ExitFinalize(entry,
                 newMasterPageTable,
                 bootInfo,
                 temporaryStack + TEMPORY_STACK_SIZE);
}
