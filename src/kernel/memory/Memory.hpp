#pragma once
#include <memory/FrameStack.hpp>
#include <setup/HandOverState.hpp>
#include <memory/ArchRegions.hpp>
#include <mutex>

class Memory
{
public:
    static const size_t NUM_STACKS = hos_v1::DMAZone::COUNT;
public: /* TODO: make private */
    FrameStack usedFrameStack[NUM_STACKS];
    FrameStack freshFrameStack[NUM_STACKS];
    KernelVirtualAddress heapEnd{KernelHeapRegion.start};
    KernelVirtualAddress halfwayDeallocatedHeapEnd{KernelHeapRegion.start};
    mutex heapLock;
    mutex frameManagementLock;
    mutex kernelPagingLock;

    void recyclePhysicalFrame(size_t stackIndex);

public:
    Memory(const hos_v1::System &handOver);

    PhysicalAddress allocatePhysicalFrame();
    void freePhysicalFrame(PhysicalAddress address);

    KernelVirtualAddress allocateVirtualArea(size_t length, size_t align = 0);
    KernelVirtualAddress resizeVirtualArea(KernelVirtualAddress address, size_t length);
    void freeVirtualArea(KernelVirtualAddress address);

    static Memory *instance();

    KernelVirtualAddress resizeHeapArea(ssize_t change);
};
