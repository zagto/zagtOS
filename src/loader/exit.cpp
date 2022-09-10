#include <exit.hpp>
#include <Paging.hpp>
#include <iostream>

#define TEMPORY_STACK_SIZE 128 // in size_ts

alignas (__uint128_t) size_t temporaryStack[TEMPORY_STACK_SIZE][512];
size_t KernelEntryAddress = 0;
hos_v1::System *BootInfo = nullptr;

void ExitToKernel(size_t processorID, size_t hardwareID) {
    assert(processorID < 512);
    if (processorID == 0) {
        cout << "Temporary stack at: " << reinterpret_cast<size_t>(&temporaryStack) << endl;
        cout << "kernel entry at: " << KernelEntryAddress << endl;
        cout << "Handover master page table at: " << reinterpret_cast<size_t>(HandOverMasterPageTable) << endl;
        cout << "Handover data at: " << reinterpret_cast<size_t>(BootInfo) << endl;
    }

    ExitFinalize(KernelEntryAddress,
                 HandOverMasterPageTable,
                 BootInfo,
                 temporaryStack[processorID] + TEMPORY_STACK_SIZE,
                 processorID,
                 hardwareID);
}
