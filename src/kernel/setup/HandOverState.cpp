#include <common/common.hpp>
#include <setup/HandOverState.hpp>
#include <processes/Thread.hpp>
#include <processes/MemoryArea.hpp>
#include <processes/Port.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Scheduler.hpp>

void hos_v1::System::decodeProcesses() {
    vector<shared_ptr<::Thread>> allThreads(numThreads);
    for (size_t index = 0; index < numThreads; index++) {
        allThreads[index] = make_shared<::Thread>(threads[index]);
    }

    vector<shared_ptr<::Port>> allPorts(numPorts);
    for (size_t index = 0; index < numPorts; index++) {
        allPorts[index] = make_shared<::Port>(ports[index], allThreads);
    }

    vector<shared_ptr<::MemoryArea>> allMemoryAreas(numMemoryAreas);
    for (size_t index = 0; index < numMemoryAreas; index++) {
        allMemoryAreas[index] = make_shared<::MemoryArea>(memoryAreas[index]);
    }

    vector<shared_ptr<::Process>> allProcesses(numProcesses);
    for (size_t index = 0; index < numProcesses; index++) {
        allProcesses[index] = make_shared<::Process>(processes[index],
                                                     allThreads,
                                                     allPorts,
                                                     allMemoryAreas);
    }
    void insertAllProcessPointersAfterKernelHandover();

    /* Now that processes are created, we can insert the pointers to them into the threads and
     * ports */
    for (const shared_ptr<::Process> &process : allProcesses) {
        process->handleManager.insertAllProcessPointersAfterKernelHandover(process);
    }

    /* TODO: Sanity check that all our elements are now referenced somewhere */

    for (const shared_ptr<::Thread> &thread: allThreads) {
        Scheduler::schedule(thread.get());
    }
}
