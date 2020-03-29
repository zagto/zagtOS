#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <memory/PlatformRegions.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Port.hpp>
#include <processes/Message.hpp>


Process::Process(ELF elf, Thread::Priority initialPrioriy, Message &runMessage):
        mappedAreas(this) {
    lock_guard lg(pagingLock);

    masterPageTable = new PagingContext(this);

    for (size_t index = 0; index < elf.numSegments(); index++) {
        elf::Segment segment = elf.segment(index);
        if (segment.type() == elf::Segment::TYPE_LOAD) {
            Region region = segment.regionInMemory();

            MappedArea *ma = new MappedArea(this, region, segment.permissions());
            mappedAreas.insert(ma);

            segment.load(this);
        }
    }

    mappedAreas.insert(new MappedArea(this, UserStackRegion, Permissions::WRITE));

    UserVirtualAddress masterTLSBase{0};
    size_t tlsSize{0};
    if (elf.hasTLS()) {
        tlsSize = elf.tlsSegment().length();
        masterTLSBase = elf.tlsSegment().address();
    }

    MappedArea *tlsArea = mappedAreas.addNew(tlsSize + THREAD_STRUCT_AREA_SIZE, Permissions::WRITE);
    if (tlsArea == nullptr) {
        cout << "TODO: whatever should happen if there is no memory" << endl;
        Panic();
    }

    UserVirtualAddress tlsBase(tlsArea->region.start + THREAD_STRUCT_AREA_SIZE);
    if (elf.hasTLS()) {
        elf.tlsSegment().load(this, tlsBase);
    }

    runMessage.transfer();

    Thread *mainThread = new Thread(this,
                                    elf.entry(),
                                    initialPrioriy,
                                    runMessage.infoAddress(),
                                    tlsBase,
                                    masterTLSBase,
                                    tlsSize);

    threads.push_back(mainThread);
    CurrentProcessor->scheduler.add(mainThread);
}

void Process::activate() {
    if (CurrentProcessor->currentProcess == this) {
        return;
    }
    masterPageTable->activate();
    CurrentProcessor->currentProcess = this;
}

PhysicalAddress Process::allocateFrame(UserVirtualAddress address,
                                    Permissions permissions) {
    assert(permissions == Permissions::WRITE
           || permissions == Permissions::EXECUTE
           || permissions == Permissions::NONE);

    PhysicalAddress physical = CurrentSystem.memory.allocatePhysicalFrame();
    masterPageTable->map(address, physical, permissions);
    return physical;
}

bool Process::handlePageFault(UserVirtualAddress address) {
    UserVirtualAddress pageAddress{align(address.value(), PAGE_SIZE, AlignDirection::DOWN)};
    lock_guard lg(pagingLock);

    MappedArea *ma = mappedAreas.findMappedArea(address);
    if (ma) {
        return ma->handlePageFault(pageAddress);
    } else {
        return false;
    }
}

void Process::removeThread(Thread *thread) {
    cout << "removing thread from process" << endl;
    threads.remove(thread);
    cout << "done" << endl;
}

/*bool Process::receiveMessage(Message *msg) {
    assert(pagingLock.isLocked());

    bool valid;
    size_t index;
    Region region = mappedAreas.findFreeRegion(size, valid, index);
    if (!valid) {
        return false;
    }
}*/

size_t Process::runMessageAddress() {
    return runMessageRegion.start + sizeof(UserMessageInfo);
}
