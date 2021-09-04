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
    Status accessUserSpace(uint8_t *buffer,
                           size_t start,
                           size_t length,
                           PagingContext::AccessOperation accOp,
                           bool requireWritePermissions);

public:
    ProcessAddressSpace addressSpace;
    HandleManager handleManager;
    vector<uint8_t> logName;
    FutexManager futexManager;

    /* The handover constructor is the only constructor you should take the result of and put it
     * into a shared pointer to pass it to the Threads. The other constructors do this by
     * themselves. DO NEVER create a shared_ptr of their results to prevent shared_ptr
     * duplication */
    Process(const hos_v1::Process &handOver,
            const vector<shared_ptr<Thread>> &allThreads,
            const vector<shared_ptr<Port>> &allPorts,
            const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
            Status &status);
    Process(Process &sourceProcess,
            vector<SpawnProcessSection> &sections,
            optional<SpawnProcessSection> &TLSSection,
            UserVirtualAddress entryAddress,
            Thread::Priority initialPrioriy,
            Message &runMessage,
            vector<uint8_t> &logName,
            Status &status);
    ~Process();
    void activate();
    void freeFrame(UserVirtualAddress address);

    bool canAccessPhysicalMemory() const;

    size_t runMessageAddress();
    Status crash(const char *message, Thread *crashedThread);
    Status exit();
};

shared_ptr<Process> CurrentProcess();
