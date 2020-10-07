#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <memory/ArchRegions.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Port.hpp>
#include <processes/Message.hpp>


Process::Process(const hos_v1::Process &handOver,
        const vector<shared_ptr<Thread>> &allThreads,
        const vector<shared_ptr<Port>> &allPorts,
        const vector<shared_ptr<SharedMemory>> &allSharedMemories) :
    mappedAreas(this, handOver),
    handleManager(handOver, allThreads, allPorts, allSharedMemories),
    futexManager(handOver.localFutexes, handOver.numLocalFutexes, allThreads)
{
    logName.resize(handOver.numLogNameChars);
    memcpy(logName.data(), handOver.logName, handOver.numLogNameChars);
    pagingContext = new PagingContext(this, handOver.pagingContext);
}


Process::Process(Process &sourceProcess,
                 vector<SpawnProcessSection> &sections,
                 optional<SpawnProcessSection> &TLSSection,
                 UserVirtualAddress entryAddress,
                 Thread::Priority initialPrioriy,
                 Message &runMessage,
                 vector<uint8_t> logName):
        mappedAreas(this),
        logName{move(logName)} {
    scoped_lock lg(pagingLock);

    pagingContext = new PagingContext(this);

    for (const auto &section: sections) {
        MappedArea *ma = new MappedArea(this, section.region(), section.permissions());
        mappedAreas.insert(ma);

        bool success = copyFromOhterUserSpace(section.address,
                                              sourceProcess,
                                              section.dataAddress,
                                              section.dataSize,
                                              false);
        assert(success);
    }

    mappedAreas.insert(new MappedArea(this, UserStackRegion, Permissions::READ_WRITE));

    UserVirtualAddress masterTLSBase{0};
    size_t TLSSize{0};
    if (TLSSection) {
        TLSSize = TLSSection->sizeInMemory;
        masterTLSBase = TLSSection->address;
    }

    MappedArea *tlsArea = mappedAreas.addNew(TLSSize + THREAD_STRUCT_AREA_SIZE, Permissions::READ_WRITE);
    if (tlsArea == nullptr) {
        cout << "TODO: whatever should happen if there is no memory" << endl;
        Panic();
    }

    UserVirtualAddress tlsBase(tlsArea->region.start + THREAD_STRUCT_AREA_SIZE);
    if (TLSSection) {
        bool success = copyFromOhterUserSpace(tlsArea->region.start,
                                              sourceProcess,
                                              TLSSection->dataAddress,
                                              TLSSection->dataSize,
                                              false);
        assert(success);
    }

    runMessage.setDestinationProcess(this);
    bool success = runMessage.transfer();
    assert(success);

    auto mainThread = make_shared<Thread>(shared_ptr<Process>(this),
                                          entryAddress,
                                          initialPrioriy,
                                          UserStackRegion.start,
                                          runMessage.infoAddress,
                                          tlsBase,
                                          masterTLSBase,
                                          TLSSize);

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
