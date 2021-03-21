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
        const vector<shared_ptr<MemoryArea> > &allMemoryAreas,
        Status &status) :
    mappedAreas(this, allMemoryAreas, handOver),
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

    Result result = make_raw<PagingContext>(this, handOver.pagingContext);
    if (!result) {
        status = result.status();
        return;
    }
    pagingContext = *result;
}


Process::Process(Process &sourceProcess,
                 vector<SpawnProcessSection> &sections,
                 optional<SpawnProcessSection> &TLSSection,
                 UserVirtualAddress entryAddress,
                 Thread::Priority initialPrioriy,
                 Message &runMessage,
                 vector<uint8_t> &logName,
                 Status &status):
        mappedAreas(this),
        logName{move(logName)},
        futexManager(status) {
    if (!status) {
        return;
    }

    scoped_lock lg(pagingLock);

    Result pagingContextResult = make_raw<PagingContext>(this);
    if (!pagingContextResult) {
        status = pagingContextResult.status();
        return;
    }
    pagingContext = *pagingContextResult;

    for (const auto &section: sections) {
        Result result = mappedAreas.addNew(section.region(), section.permissions());
        if (!result) {
            status = result.status();
            return;
        }

        status = copyFromOhterUserSpace(section.address,
                                              sourceProcess,
                                              section.dataAddress,
                                              section.dataSize,
                                              false);
        if (!status) {
            return;
        }
    }

    // TODO: stack border
    Result result = mappedAreas.addNew(UserStackRegion, Permissions::READ_WRITE);
    if (!result) {
        status = result.status();
        return;
    }

    UserVirtualAddress masterTLSBase{0};
    size_t TLSSize{0};
    if (TLSSection) {
        TLSSize = TLSSection->sizeInMemory;
        masterTLSBase = TLSSection->address;
    }

    result = mappedAreas.addNew(TLSSize + THREAD_STRUCT_AREA_SIZE, Permissions::READ_WRITE);
    if (!result) {
        status = result.status();
        return;
    }
    MappedArea *tlsArea = *result;

    UserVirtualAddress tlsBase(tlsArea->region.start + THREAD_STRUCT_AREA_SIZE);
    if (TLSSection) {
        status = copyFromOhterUserSpace(tlsArea->region.start,
                                        sourceProcess,
                                        TLSSection->dataAddress,
                                        TLSSection->dataSize,
                                        false);
        if (!status) {
            return;
        }
    }

    runMessage.setDestinationProcess(this);
    status = runMessage.transfer();
    if (!status) {
        return;
    }

    shared_ptr<Process> sharedProcess = shared_ptr<Process>(this, status);
    if (!status) {
        return;
    }

    Result mainThread = make_shared<Thread>(sharedProcess,
                                            entryAddress,
                                            initialPrioriy,
                                            /* ensure valid UserVirtualAddress */
                                            UserStackRegion.end() - 1,
                                            runMessage.infoAddress,
                                            tlsBase,
                                            masterTLSBase,
                                            TLSSize);
    if (!mainThread) {
        status = mainThread.status();
        return;
    }

    Result handle = handleManager.addThread(*mainThread);
    if (handle) {
        Scheduler::schedule(mainThread->get());
    } else {
        status = handle.status();
    }
}

Process::~Process() {
    cout << "destroying Process..." << endl;

    cout << "TODO: actually unmap stuff" << endl;
    Panic();

    cout << "Process terminated" << endl;
}

Status Process::handlePageFault(UserVirtualAddress address) {
    UserVirtualAddress pageAddress{align(address.value(), PAGE_SIZE, AlignDirection::DOWN)};
    scoped_lock lg(pagingLock);

    MappedArea *ma = mappedAreas.findMappedArea(address);
    if (ma) {
        return ma->handlePageFault(pageAddress);
    } else {
        return Status::BadUserSpace();
    }
}

bool Process::canAccessPhysicalMemory() const {
    /* TODO: introduce process flags */
    return true;
}

void Process::crash(const char *message, Thread *crashedThread) {
    /* TODO: throw an exception? This method can be called from anywhere so we need to make sure all
    locks are clear */
    cout << "Terminating process for reason: " << message << endl;
    if (crashedThread != nullptr) {
        Status status = coreDump(crashedThread);
        if (!status) {
            cout << "unable to core dump because of Exception" << endl;
        }
    }
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

shared_ptr<Process> CurrentProcess() {
    return CurrentProcessor->scheduler.activeThread()->process;
}
