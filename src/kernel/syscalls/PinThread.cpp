#include <syscalls/PinThread.hpp>
#include <system/Processor.hpp>

size_t PinThread(const shared_ptr<Process> &process,
                         size_t processorID,
                         size_t,
                         size_t,
                         size_t,
                         size_t) {
    static constexpr size_t PROCESSOR_ID_NONE = static_cast<size_t>(-1);
    if (processorID == PROCESSOR_ID_NONE) {
        cout << "Un-pinning thread" << endl;
        CurrentThread()->pinToProcessor(nullptr);
    } else if (processorID < CurrentSystem.numProcessors) {
        cout << "Pinning Thread to Processor " << processorID << endl;
        CurrentThread()->pinToProcessor(&Processors[processorID]);
    } else {
        cout << "PinThread: Processor ID " << processorID << " is out of range" << endl;
        throw BadUserSpace(process);
    }
    return 0;
}

