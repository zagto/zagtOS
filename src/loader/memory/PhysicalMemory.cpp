#include <memory/PhysicalMemory.hpp>
#include <MemoryMap.hpp>
#include <iostream>
#include <Paging.hpp>
#include <common/utils.hpp>

static frameStack::Node *const FRAMESTACK_NULL =
        reinterpret_cast<frameStack::Node *>(PhysicalAddress::Null);

FrameStack DirtyFrameStack[hos_v1::DMAZone::COUNT];
FrameStack CleanFrameStack[hos_v1::DMAZone::COUNT];

/* Some structures need to be placed at the beginning of memory to make secondary Processor start
 * possible on x86_64. */
#ifdef __x86_64__
PhysicalAddress SecondaryProcessorEntry{PhysicalAddress::Null};
extern PageTable *HandOverMasterPageTable;
static bool secondaryProcessorEntryFound{false};
static bool handOvermasterPageTableFound{false};

static const size_t PROCESSOR_ENTRY_MAX{1ul << 16};
static const size_t HAND_OVER_MASTER_PAGE_TABLE_MAX{1ul << 32};
#endif

/* returns the maximum physical address that is part of usable memory */
PhysicalAddress InitPhysicalFrameManagement() {
    for (FrameStack &stack: DirtyFrameStack) {
        stack.head = FRAMESTACK_NULL;
        stack.addIndex = 0;
    }
    for (FrameStack &stack: CleanFrameStack) {
        stack.head = FRAMESTACK_NULL;
        stack.addIndex = 0;
    }

    optional<Region> region;
    PhysicalAddress maxPhysicalAddress{0};

    region = memoryMap::firstAvailableRegion();
    if (!region) {
        cout << "No available Memory" << endl;
        Halt();
    }

    /* Initialize Frame Stacks */
    for (FrameStack &stack: DirtyFrameStack) {
        stack.head = FRAMESTACK_NULL;
        stack.addIndex = frameStack::Node::NUM_ENTRIES;
    }
    for (FrameStack &stack: CleanFrameStack) {
        stack.head = FRAMESTACK_NULL;
        stack.addIndex = frameStack::Node::NUM_ENTRIES;
    }
    size_t address = region->start;

    while (region) {
        if (region->end() > maxPhysicalAddress.value()) {
            maxPhysicalAddress = region->end();
        }

        while (address < region->end()) {
#ifdef __x86_64__
            /* HandOverMasterPageTable and SecondaryProcessorEntry have special physical location
             * requirements to be usable in the real mode entry code.
             * Reserve suitable frames for these */
            if (!secondaryProcessorEntryFound && address < PROCESSOR_ENTRY_MAX) {
                SecondaryProcessorEntry = address;
                secondaryProcessorEntryFound = true;
            } else if (!handOvermasterPageTableFound && address < HAND_OVER_MASTER_PAGE_TABLE_MAX) {
                HandOverMasterPageTable = (PageTable *)address;
                handOvermasterPageTableFound = true;
            } else {
                FreePhysicalFrame(address);
            }
#else
            FreePhysicalFrame(address);
#endif
            address += PAGE_SIZE;
        }

        /* prepare for next round */
        region = memoryMap::nextAvailableRegion();
        if (region) {
            address = region->start;
        }
    }

#ifdef __x86_64__
    if (!secondaryProcessorEntryFound) {
        cout << "Unable to find frame for secondary processor entry code" << endl;
        Halt();
    }
#endif
    return maxPhysicalAddress;
}


PhysicalAddress AllocatePhysicalFrame(int zone) {
    PhysicalAddress address;
    for (int stackIndex = zone; stackIndex >= 0; stackIndex--) {
        if (!DirtyFrameStack[stackIndex].isEmpty()) {
            address = DirtyFrameStack[stackIndex].pop();
            break;
        }
    }
    if (address == PhysicalAddress::Null) {
        cout << "out of memory" << endl;
        Panic();
    }
    ClearFrame(reinterpret_cast<void *>(address.value()));
    return address;
}


void FreePhysicalFrame(PhysicalAddress frame) {
    for (size_t stackIndex = 0; stackIndex < hos_v1::DMAZone::COUNT; stackIndex++) {
        if (frame.value() <= hos_v1::DMAZoneMax[stackIndex]) {
            DirtyFrameStack[stackIndex].push(frame);
            return;
        }
    }
    cout << "should not reach here" << endl;
    Halt();
}


void ClearFrame(void *frame) {
    memset(frame, 0, PAGE_SIZE);
}
