#include <exit.hpp>
#include <Paging.hpp>
#include <iostream>

#define TEMPORY_STACK_SIZE 128 // in size_ts

alignas (__uint128_t) size_t temporaryStack[TEMPORY_STACK_SIZE][512];
size_t KernelEntryAddress = 0;
hos_v1::System *BootInfo = nullptr;

void ExitToKernel(size_t processorID, size_t hardwareID, hos_v1::PagingContext *pagingContext) {
    assert(processorID < 512);
    if (processorID == 0) {
        cout << "kernel entry at: " << KernelEntryAddress << endl;
        cout << "Temporary stack at: " << reinterpret_cast<size_t>(&temporaryStack) << endl;
        cout << "Handover data at: " << reinterpret_cast<size_t>(BootInfo) << endl;
    }

    uint64_t r;
    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    uint64_t b=r&0xF;
    cout << "supported bits: " << b << endl;
    //if(r&(0xF<<28)/*4k*/ || b<0b101/*36 bits*/) {
    //    cout << "ERROR: 4k granule or 63 bit address space not supported" << endl;
    //    Halt();
    //}


    ExitFinalize(KernelEntryAddress,
                 pagingContext,
                 BootInfo,
                 temporaryStack[processorID] + TEMPORY_STACK_SIZE,
                 processorID,
                 hardwareID);
}
