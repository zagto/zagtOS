#include <common/common.hpp>
#include <setup/HandOverState.hpp>
#include <processes/Thread.hpp>
#include <processes/MemoryArea.hpp>
#include <processes/HandleManager.hpp>
#include <processes/Process.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Scheduler.hpp>

#include <system/Processor.hpp>

void hos_v1::System::decodeProcesses() {
    /* TODO: deal with OOM */
    vector<shared_ptr<::Thread>> allThreads(numThreads);
    vector<shared_ptr<::Port>> allPorts(numPorts);
    vector<shared_ptr<::MemoryArea>> allMemoryAreas(numMemoryAreas);
    vector<shared_ptr<::Process>> allProcesses(numProcesses);
    vector<shared_ptr<::EventQueue>> allEventQueues(numEventQueues);
    vector<::Frame *> allFrames(numFrames, nullptr);

    for (size_t index = 0; index < numThreads; index++) {
        allThreads[index] = make_shared<::Thread>(threads[index]);
    }
    for (size_t index = 0; index < numEventQueues; index++) {
        allEventQueues[index] = make_shared<::EventQueue>(eventQueues[index], allThreads);
    }
    for (size_t index = 0; index < numPorts; index++) {
        allPorts[index] = make_shared<::Port>(allEventQueues[ports[index].eventQueueID],
                                              ports[index].tag);
    }
    for (size_t index = 0; index < numFrames; index++) {
        allFrames[index] = new ::Frame(frames[index]);
    }
    for (size_t index = 0; index < numMemoryAreas; index++) {
        allMemoryAreas[index] = make_shared<::MemoryArea>(memoryAreas[index], allFrames);
    }
    for (size_t index = 0; index < numProcesses; index++) {
        allProcesses[index] = make_shared<::Process>(processes[index],
                                                     allThreads,
                                                     allPorts,
                                                     allMemoryAreas,
                                                     allEventQueues);
    }

    /* Now that processes are created, we can insert the pointers to them into the threads and
     * ports */
    for (const shared_ptr<::Process> &process : allProcesses) {
        process->handleManager.insertAllProcessPointersAfterKernelHandover(process);
        process->self = process;
    }

    /* TODO: Sanity check that all our elements are now referenced somewhere */

    for (const shared_ptr<::Thread> &thread: allThreads) {
        thread->process->allThreads.append(thread.get());
        Scheduler::schedule(thread.get(), false);
    }

}
