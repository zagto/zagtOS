#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <memory/FrameStack.hpp>
#include <setup/BootInfo.hpp>
#include <memory/PlatformRegions.hpp>
#include <lib/Lock.hpp>

class Memory
{
public: /* TODO: make private */
    FrameStack usedFrameStack;
    FrameStack freshFrameStack;
    KernelVirtualAddress heapEnd{KernelHeapRegion.start};
    KernelVirtualAddress halfwayDeallocatedHeapEnd{KernelHeapRegion.start};
    Lock heapLock;
    Lock frameManagementLock;
    Lock kernelPagingLock;

    void recyclePhysicalFrame();

public:
    Memory(BootInfo *bootInfo);

    PhysicalAddress allocatePhysicalFrame();
    void freePhysicalFrame(PhysicalAddress address);

    KernelVirtualAddress allocateVirtualArea(size_t length, size_t align = 0);
    KernelVirtualAddress resizeVirtualArea(KernelVirtualAddress address, size_t length);
    void freeVirtualArea(KernelVirtualAddress address);

    static Memory *instance();

    KernelVirtualAddress resizeHeapArea(ssize_t change);
};

#endif // MEMORY_HPP
