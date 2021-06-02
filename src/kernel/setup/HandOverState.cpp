#include <common/common.hpp>
#include <setup/HandOverState.hpp>
#include <processes/Thread.hpp>
#include <processes/MemoryArea.hpp>
#include <processes/Port.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Scheduler.hpp>

#include <system/Processor.hpp>

void hos_v1::System::decodeProcesses() {
    /* TODO: deal with OOM */
    Status status = Status::OK();
    vector<shared_ptr<::Thread>> allThreads(numThreads, status);
    assert(status);
    vector<shared_ptr<::Port>> allPorts(numPorts, status);
    assert(status);
    vector<shared_ptr<::MemoryArea>> allMemoryAreas(numMemoryAreas, status);
    assert(status);
    vector<shared_ptr<::Process>> allProcesses(numProcesses, status);
    assert(status);
    vector<::Frame *> allFrames(numFrames, nullptr, status);
    assert(status);

    for (size_t index = 0; index < numThreads; index++) {
        allThreads[index] = *make_shared<::Thread>(threads[index]);
    }
    for (size_t index = 0; index < numPorts; index++) {
        allPorts[index] = *make_shared<::Port>(ports[index], allThreads);
    }
    for (size_t index = 0; index < numFrames; index++) {
        allFrames[index] = *make_raw<::Frame>(frames[index]);
    }
    for (size_t index = 0; index < numMemoryAreas; index++) {
        allMemoryAreas[index] = *make_shared<::MemoryArea>(memoryAreas[index], allFrames);
    }
    for (size_t index = 0; index < numProcesses; index++) {
        allProcesses[index] = *make_shared<::Process>(processes[index],
                                                     allThreads,
                                                     allPorts,
                                                     allMemoryAreas);
    }

    /* Now that processes are created, we can insert the pointers to them into the threads and
     * ports */
    for (const shared_ptr<::Process> &process : allProcesses) {
        process->handleManager.insertAllProcessPointersAfterKernelHandover(process);
    }

    /* TODO: Sanity check that all our elements are now referenced somewhere */

    for (const shared_ptr<::Thread> &thread: allThreads) {
        Scheduler::schedule(thread.get());

        cout << "1 -- On decode User cs of thread " << thread.get() <<" is " << thread->registerState.cs << endl;
    }

}
