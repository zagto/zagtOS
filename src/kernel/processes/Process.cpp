#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <memory/ArchRegions.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Port.hpp>
#include <processes/Message.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>


Process::Process(const hos_v1::Process &handOver,
                 const vector<shared_ptr<Thread>> &allThreads,
                 const vector<shared_ptr<Port>> &allPorts,
                 const vector<shared_ptr<MemoryArea> > &allMemoryAreas) :
        addressSpace(handOver, allMemoryAreas),
        handleManager(*this, handOver, allThreads, allPorts, allMemoryAreas),
        futexManager(handOver.localFutexes, handOver.numLocalFutexes, allThreads) {

    logName.resize(handOver.numLogNameChars);
    memcpy(logName.data(), handOver.logName, handOver.numLogNameChars);
}


Process::Process(Process &sourceProcess,
                 vector<SpawnProcessSection> &sections,
                 optional<SpawnProcessSection> &TLSSection,
                 UserVirtualAddress entryAddress,
                 Thread::Priority initialPrioriy,
                 Message &runMessage,
                 vector<uint8_t> &logName):
        handleManager(*this),
        logName{move(logName)} {

    for (const auto &section: sections) {
        addressSpace.addAnonymous(section.region(),
                                  section.permissions(),
                                  true,
                                  false);

        addressSpace.copyFromOhter(section.address,
                                   sourceProcess.addressSpace,
                                   section.dataAddress,
                                   section.dataSize,
                                   false);
    }

    UserVirtualAddress masterTLSBase{0};
    size_t TLSSize{0};
    if (TLSSection) {
        TLSSize = TLSSection->sizeInMemory;
        masterTLSBase = TLSSection->address;
    }

    UserVirtualAddress tlsAddress = addressSpace.addAnonymous(
                TLSSize + hos_v1::THREAD_STRUCT_AREA_SIZE,
                Permissions::READ_WRITE);

    UserVirtualAddress tlsBase = tlsAddress + hos_v1::THREAD_STRUCT_AREA_SIZE;
    if (TLSSection) {
        addressSpace.copyFromOhter(tlsAddress.value(),
                                   sourceProcess.addressSpace,
                                   TLSSection->dataAddress,
                                   TLSSection->dataSize,
                                   false);
    }

    runMessage.setDestinationProcess(this);
    runMessage.transfer();

    shared_ptr<Process> sharedProcess = shared_ptr<Process>(this);
    self = sharedProcess;

    auto mainThread = make_shared<Thread>(sharedProcess,
                                          entryAddress,
                                          initialPrioriy,
                                          /* ensure valid UserVirtualAddress */
                                          UserSpaceRegion.end() - 1,
                                          runMessage.infoAddress,
                                          tlsBase,
                                          masterTLSBase,
                                          TLSSize);
    handleManager.addThread(mainThread);
    Scheduler::schedule(mainThread.get(), true);
}

Process::~Process() {
    cout << "destroying Process";
    for (auto c : logName) {
        cout.output(c);
    }
    cout << "..." << endl;

    cout << "TODO: actually unmap stuff" << endl;
    Panic();    

    cout << "Process terminated" << endl;
}

bool Process::canAccessPhysicalMemory() const noexcept {
    /* TODO: introduce process flags */
    return true;
}

/* will return ThreadKilled when the current Thread is killed, and OK otherwise */
void Process::crash(const char *message) {
    assert(KernelInterruptsLock.isLocked());
    cout << "Terminating process for reason: " << message << endl;

    exit();
}

/* will return ThreadKilled when the current Thread is killed, and OK otherwise */
void Process::exit() {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl1(allThreadsLock);

    for (Thread *thread: allThreads) {
        if (thread != CurrentThread()) {
            thread->terminate();
        }
    }

    if (CurrentThread()->process.get() == this) {
        throw ThreadKilled();
    } else {
        return;
    }
}

shared_ptr<Process> CurrentProcess() {
    scoped_lock sl(KernelInterruptsLock);
    return CurrentProcessor()->activeThread()->process;
}
