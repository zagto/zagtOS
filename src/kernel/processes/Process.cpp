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
        const vector<shared_ptr<MemoryArea> > &allMemoryAreas,
        Status &status) :
    addressSpace(handOver, allMemoryAreas, status),
    handleManager(handOver, allThreads, allPorts, allMemoryAreas, status),
    futexManager(handOver.localFutexes, handOver.numLocalFutexes, allThreads, status)
{
    if (!status) {
        return;
    }

    status = logName.resize(handOver.numLogNameChars);
    if (!status) {
        return;
    }
    memcpy(logName.data(), handOver.logName, handOver.numLogNameChars);
}


Process::Process(Process &sourceProcess,
                 vector<SpawnProcessSection> &sections,
                 optional<SpawnProcessSection> &TLSSection,
                 UserVirtualAddress entryAddress,
                 Thread::Priority initialPrioriy,
                 Message &runMessage,
                 vector<uint8_t> &logName,
                 Status &status):
        addressSpace(status),
        logName{move(logName)},
        futexManager(status) {
    if (!status) {
        cout << "Process constructor cancelled: " << status << endl;
        return;
    }

    for (const auto &section: sections) {
        Result result = addressSpace.addAnonymous(section.region(),
                                                  section.permissions(),
                                                  true,
                                                  false);
        if (!result) {
            status = result.status();
            cout << "Could not add section to address space of new Process: " << status << endl;
            return;
        }

        status = addressSpace.copyFromOhter(section.address,
                                            sourceProcess.addressSpace,
                                            section.dataAddress,
                                            section.dataSize,
                                            false);
        if (!status) {
            cout << "Could not load section of new Process: " << status << endl;
            return;
        }
    }

    UserVirtualAddress masterTLSBase{0};
    size_t TLSSize{0};
    if (TLSSection) {
        TLSSize = TLSSection->sizeInMemory;
        masterTLSBase = TLSSection->address;
    }

    Result tlsAddress = addressSpace.addAnonymous(TLSSize + hos_v1::THREAD_STRUCT_AREA_SIZE,
                                                  Permissions::READ_WRITE);
    if (!tlsAddress) {
        status = tlsAddress.status();
        cout << "Could not add TLS to address space of new Process: " << status << endl;
        return;
    }

    UserVirtualAddress tlsBase = *tlsAddress + hos_v1::THREAD_STRUCT_AREA_SIZE;
    if (TLSSection) {
        status = addressSpace.copyFromOhter(tlsAddress->value(),
                                            sourceProcess.addressSpace,
                                            TLSSection->dataAddress,
                                            TLSSection->dataSize,
                                            false);
        if (!status) {
            cout << "Could not load TLS segment for new Process: " << status << endl;
            return;
        }
    }

    runMessage.setDestinationProcess(this);
    status = runMessage.transfer();
    if (!status) {
        cout << "Could not transfer run message for new Process: " << status << endl;
        return;
    }

    shared_ptr<Process> sharedProcess = shared_ptr<Process>(this, status);
    if (!status) {
        cout << "Could not create shared_ptr for new Process: " << status << endl;
        return;
    }

    Result mainThread = make_shared<Thread>(sharedProcess,
                                            entryAddress,
                                            initialPrioriy,
                                            /* ensure valid UserVirtualAddress */
                                            UserSpaceRegion.end() - 1,
                                            runMessage.infoAddress,
                                            tlsBase,
                                            masterTLSBase,
                                            TLSSize);
    if (!mainThread) {
        cout << "Could not create main thread for new Process: " << status << endl;
        status = mainThread.status();
        return;
    }

    Result handle = handleManager.addThread(*mainThread);
    if (handle) {
        Scheduler::schedule(mainThread->get(), true);
    } else {
        status = handle.status();
        cout << "Could not add Thread to HandleManager of new Process: " << status << endl;
    }
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

bool Process::canAccessPhysicalMemory() const {
    /* TODO: introduce process flags */
    return true;
}

Status Process::crash(const char *message) {
    assert(KernelInterruptsLock.isLocked());
    cout << "Terminating process for reason: " << message << endl;

    return exit();
}

Status Process::exit() {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl1(allThreadsLock);

    for (Thread *thread: allThreads) {
        if (thread != CurrentThread()) {
            thread->terminate();
        }
    }

    if (CurrentThread()->process.get() == this) {
        return Status::ThreadKilled();
    } else {
        return Status::OK();
    }
}

shared_ptr<Process> CurrentProcess() {
    return CurrentProcessor->scheduler.activeThread()->process;
}
