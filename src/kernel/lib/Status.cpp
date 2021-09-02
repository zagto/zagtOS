#include <lib/Status.hpp>
#include <log/Logger.hpp>

Logger &operator<<(Logger &logger, Status &status) {
    switch (status.type) {
    case OK:
        logger << "OK";
        break;
    case OutOfMemory:
        logger << "Out of Memory";
        break;
    case OutOfKernelHeap:
        logger << "Out of Kernel Heap";
        break;
    case BadUserSpace:
        logger << "Bad User Space";
        break;
    case ThreadKilled:
        logger << "Thread Killed";
        break;
    case DiscardStateAndSchedule:
        logger << "Discard Kernel State and Schedule";
        break;
    case NonInitialized:
        cout << "Tried to print not initialized Status" << endl;
        Panic();
    }
    return logger;
}
