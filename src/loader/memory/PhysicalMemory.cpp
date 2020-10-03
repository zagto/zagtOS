#include <memory/PhysicalMemory.hpp>
#include <MemoryMap.hpp>
#include <log/Logger.hpp>
#include <Paging.hpp>
#include <common/utils.hpp>

static frameStack::Node *const FRAMESTACK_NULL =
        reinterpret_cast<frameStack::Node *>(PhysicalAddress::NULL);

FrameStack DirtyFrameStack(FRAMESTACK_NULL, 0);
FrameStack CleanFrameStack(FRAMESTACK_NULL, 0);

PhysicalAddress SecondaryProcessorEntry{PhysicalAddress::NULL};
static bool secondaryProcessorEntryFound{false};
static bool handOvermasterPageTableFound{false};

/* Some structures need to be placed at the beginning of memory to make secondary Processor start
 * possible on x86_64. */
static const size_t PROCESSOR_ENTRY_MAX{1ul << 16};
static const size_t HAND_OVER_MASTER_PAGE_TABLE_MAX{1ul << 32};

/* returns the maximum physical address that is part of usable memory */
PhysicalAddress InitPhysicalFrameManagement() {
    optional<Region> region;
    PhysicalAddress maxPhysicalAddress{0};

    region = memoryMap::firstAvailableRegion();
    if (!region) {
        cout << "No available Memory" << endl;
        Halt();
    }

    // The first available frame will be the first frame list frame
    DirtyFrameStack.head = reinterpret_cast<frameStack::Node *>(region->start);
    ClearFrame(DirtyFrameStack.head);
    DirtyFrameStack.head->next = FRAMESTACK_NULL;

    /* start with second frame because the first is our stack node frame, so it is not available */
    size_t address = region->start + PAGE_SIZE;

    while (region) {
        if (region->end() > maxPhysicalAddress.value()) {
            maxPhysicalAddress = region->end();
        }

        while (address < region->length) {
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
                DirtyFrameStack.push(address);
            }
            address += PAGE_SIZE;
        }

        /* prepare for next round */
        region = memoryMap::nextAvailableRegion();
        if (region) {
            address = region->start;
        }
    }

    if (!secondaryProcessorEntryFound) {
        cout << "Unable to find frame for secondary processor entry code" << endl;
        Halt();
    }

    /* Process Master Page Tables can be at any place in memory */
    ProcessMasterPageTable = reinterpret_cast<PageTable *>(AllocatePhysicalFrame().value());

    CleanFrameStack.head = reinterpret_cast<frameStack::Node *>(AllocatePhysicalFrame().value());
    ClearFrame(CleanFrameStack.head);
    CleanFrameStack.head->next = FRAMESTACK_NULL;

    // clean 100 frames so kernel can allocate memory before it's cleaning mechanism works
    for (size_t i = 0; i < 100; i++) {
        PhysicalAddress frame = DirtyFrameStack.pop();
        ClearFrame(reinterpret_cast<void *>(frame.value()));
        CleanFrameStack.push(frame);
    }
    return maxPhysicalAddress;
}


PhysicalAddress AllocatePhysicalFrame(void) {
    PhysicalAddress address = DirtyFrameStack.pop();
    ClearFrame(reinterpret_cast<void *>(address.value()));
    return address;
}


void FreePhysicalFrame(PhysicalAddress frame) {
    DirtyFrameStack.push(frame);
}


void ClearFrame(void *frame) {
    memset(frame, 0, PAGE_SIZE);
}
