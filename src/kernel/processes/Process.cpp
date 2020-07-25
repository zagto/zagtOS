#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <memory/PlatformRegions.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Port.hpp>
#include <processes/Message.hpp>


Process::Process(ELF elf,
                 Thread::Priority initialPrioriy,
                 Message &runMessage,
                 vector<uint8_t> logName):
        mappedAreas(this),
        logName{move(logName)} {
    scoped_lock lg(pagingLock);

    pagingContext = new PagingContext(this);

    for (size_t index = 0; index < elf.numSegments(); index++) {
        elf::Segment segment = elf.segment(index);
        if (segment.type() == elf::Segment::TYPE_LOAD) {
            Region region = segment.regionInMemory();

            MappedArea *ma = new MappedArea(this, region, segment.permissions());
            mappedAreas.insert(ma);

            segment.load(this);
        }
    }

    mappedAreas.insert(new MappedArea(this, UserStackRegion, Permissions::READ_WRITE));

    UserVirtualAddress masterTLSBase{0};
    size_t tlsSize{0};
    if (elf.hasTLS()) {
        tlsSize = elf.tlsSegment().length();
        masterTLSBase = elf.tlsSegment().address();
    }

    MappedArea *tlsArea = mappedAreas.addNew(tlsSize + THREAD_STRUCT_AREA_SIZE, Permissions::READ_WRITE);
    if (tlsArea == nullptr) {
        cout << "TODO: whatever should happen if there is no memory" << endl;
        Panic();
    }

    UserVirtualAddress tlsBase(tlsArea->region.start + THREAD_STRUCT_AREA_SIZE);
    if (elf.hasTLS()) {
        elf.tlsSegment().load(this, tlsBase);
    }

    runMessage.setDestinationProcess(this);
    bool success = runMessage.transfer();
    assert(success);

    auto mainThread = make_shared<Thread>(shared_ptr<Process>(this),
                                          elf.entry(),
                                          initialPrioriy,
                                          runMessage.infoAddress(),
                                          tlsBase,
                                          masterTLSBase,
                                          tlsSize);

    handleManager.addThread(mainThread);
    Scheduler::schedule(mainThread.get());
}

Process::~Process() {
    cout << "destroying Process..." << endl;

    cout << "TODO: actually unmap stuff" << endl;
    Panic();

    cout << "Process terminated" << endl;
}

PhysicalAddress Process::allocateFrame(UserVirtualAddress address,
                                    Permissions permissions) {
    assert(permissions == Permissions::READ_WRITE
           || permissions == Permissions::READ_EXECUTE
           || permissions == Permissions::READ);

    PhysicalAddress physical = CurrentSystem.memory.allocatePhysicalFrame();
    pagingContext->map(address, physical, permissions);
    return physical;
}

bool Process::handlePageFault(UserVirtualAddress address) {
    UserVirtualAddress pageAddress{align(address.value(), PAGE_SIZE, AlignDirection::DOWN)};
    scoped_lock lg(pagingLock);

    MappedArea *ma = mappedAreas.findMappedArea(address);
    if (ma) {
        return ma->handlePageFault(pageAddress);
    } else {
        return false;
    }
}

void Process::crash(const char *message) {
    cout << "Terminating process for reason: " << message << endl;
    exit();
}

void Process::exit() {
    // TODO: this is not very efficient
    while (true) {
        shared_ptr<Thread> thread = handleManager.extractThread();
        if (!thread) {
            return;
        }
        thread->terminate();
    }
}
