#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <memory/ArchRegions.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Message.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>


Process::Process(const hos_v1::Process &handOver,
                 const vector<shared_ptr<Thread>> &allThreads,
                 const vector<shared_ptr<Port>> &allPorts,
                 const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                 const vector<shared_ptr<EventQueue>> &allEventQueues) :
        addressSpace(handOver, allMemoryAreas),
        handleManager(*this, handOver, allThreads, allPorts, allMemoryAreas, allEventQueues),
        futexManager(handOver.localFutexes, handOver.numLocalFutexes, allThreads) {

    logName.resize(handOver.numLogNameChars);
    memcpy(logName.data(), handOver.logName, handOver.numLogNameChars);
}


Process::Process(Process &sourceProcess,
                 vector<SpawnProcessSection> &sections,
                 UserVirtualAddress entryAddress,
                 size_t tlsPointer,
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

    shared_ptr<Process> sharedProcess = shared_ptr<Process>(this);
    self = sharedProcess;

    auto mainThread = make_shared<Thread>(sharedProcess,
                                          entryAddress,
                                          initialPrioriy,
                                          /* ensure valid UserVirtualAddress */
                                          UserSpaceRegion.end() - 1,
                                          /* entry argument is set later to startup info */ 0,
                                          tlsPointer);
    uint32_t threadHandle = handleManager.add(mainThread);
    mainThread->setHandle(threadHandle);
    auto eventQueue = make_shared<EventQueue>(sharedProcess);
    uint32_t eventQueueHandle = handleManager.add(eventQueue);

    runMessage.setDestinationProcess(this);
    runMessage.setStartupInfo(threadHandle, eventQueueHandle);
    runMessage.transfer();
    mainThread->kernelStack->userRegisterState()->setEntryArgument(runMessage.infoAddress.value());
    Scheduler::schedule(mainThread.get(), true);
}

Process::~Process() {
    cout << "destroying Process";
    for (auto c : logName) {
        cout << static_cast<char>(c);
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
