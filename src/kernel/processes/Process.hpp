#pragma once

#include <mutex>
#include <queue>
#include <memory>
#include <utility>
#include <processes/Thread.hpp>
#include <processes/UUID.hpp>
#include <processes/Message.hpp>
#include <processes/HandleManager.hpp>
#include <processes/FutexManager.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <processes/ProcessAddressSpace.hpp>

class Process {
private:
    void accessUserSpace(uint8_t *buffer,
                         size_t start,
                         size_t length,
                         PagingContext::AccessOperation accOp,
                         bool requireWritePermissions);

public:
    /* should be first member for ProcessAddressSpace::process hack to work */
    ProcessAddressSpace addressSpace;

    /* our alternative to enable_shared_from_this - which we don't have in kernel */
    weak_ptr<Process> self;

    HandleManager handleManager;
    vector<uint8_t> logName;
    FutexManager futexManager;

    /* This list is used when the Process exits to quickly find all Threads to terminate */
    SpinLock allThreadsLock;
    threadList::List<&Thread::processReceptor> allThreads;

    /* The handover constructor is the only constructor you should take the result of and put it
     * into a shared pointer to pass it to the Threads. The other constructors do this by
     * themselves. DO NEVER create a shared_ptr of their results to prevent shared_ptr
     * duplication */
    Process(const hos_v1::Process &handOver,
            const vector<shared_ptr<Thread>> &allThreads,
            const vector<shared_ptr<Port>> &allPorts,
            const vector<shared_ptr<MemoryArea>> &allMemoryAreas);
    Process(Process &sourceProcess,
            vector<SpawnProcessSection> &sections,
            optional<SpawnProcessSection> &TLSSection,
            UserVirtualAddress entryAddress,
            Thread::Priority initialPrioriy,
            Message &runMessage,
            vector<uint8_t> &logName);
    Process(const Process &) = delete;
    Process &operator=(const Process &) = delete;
    ~Process();

    bool canAccessPhysicalMemory() const noexcept;

    void crash(const char *message);
    void exit();
};

shared_ptr<Process> CurrentProcess();
