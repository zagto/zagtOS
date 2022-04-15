#include <syscalls/Log.hpp>
#include <syscalls/Exit.hpp>
#include <syscalls/SendMessage.hpp>
#include <syscalls/ReceiveMessage.hpp>
#include <syscalls/CreatePort.hpp>
#include <syscalls/DeleteHandle.hpp>
#include <syscalls/Random.hpp>
#include <syscalls/Crash.hpp>
#include <syscalls/MProtect.hpp>
#include <syscalls/MMap.hpp>
#include <syscalls/MUnmap.hpp>
#include <syscalls/CreateSharedMemory.hpp>
#include <syscalls/CreateThread.hpp>
#include <syscalls/Futex.hpp>
#include <syscalls/GetTime.hpp>
#include <syscalls/CreateIOPortRange.hpp>
#include <syscalls/GetFirmwareRoot.hpp>
#include <syscalls/IOPortRead.hpp>
#include <syscalls/IOPortWrite.hpp>
#include <syscalls/CreateInterrupt.hpp>
#include <syscalls/SubscribeInterrupt.hpp>
#include <syscalls/UnsubscribeInterrupt.hpp>
#include <syscalls/ProcessedInterrupt.hpp>
#include <syscalls/WaitInterrupt.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/PinThread.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#include <interrupts/Interrupts.hpp> // dealWithException

typedef size_t SyscallFunction(const shared_ptr<Process> &, size_t, size_t, size_t, size_t, size_t);

static SyscallFunction *syscallFunctions[] = {
    /* 0 */
    nullptr,
    &Log,
    &Exit,
    &SendMessage,
    &ReceiveMessage,
    &CreatePort,
    &DeleteHandle,
    &Random,
    &Crash,
    nullptr,

    /* 10 */
    &MProtect,
    &MMap,
    &MUnmap,
    nullptr, /* msync */
    nullptr, /* mremap */
    &CreateSharedMemory,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    /* 20 */
    &CreateThread,
    nullptr, /* ExitThread */
    &Futex,
    nullptr, /* yield */
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    /* 30 */
    &GetTime,
    nullptr, /* nanosleep */
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    /* 40 */
    nullptr,
    &CreateIOPortRange,
    &GetFirmwareRoot,
    &IOPortRead,
    &IOPortWrite,
    &CreateInterrupt,
    &SubscribeInterrupt,
    &UnsubscribeInterrupt,
    &ProcessedInterrupt,
    &WaitInterrupt,

    /* 50 */
    &SpawnProcess,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    /* 60 */
    &PinThread,
};

extern "C"
Exception::Action Syscall(size_t syscallNr,
               size_t arg0,
               size_t arg1,
               size_t arg2,
               size_t arg3,
               size_t arg4) {
    Exception::Action action;

    /* scope to avoid leaking anything when calling noreturn function returnToUserMode() */
    {
        const shared_ptr<Process> process = CurrentProcess();
        Thread *thread = CurrentThread();
        assert(CurrentThread()->kernelStack->userRegisterState()->fromSyscall == 1);

        size_t result = -1ul;
        do {
            action = Exception::Action::CONTINUE;
            try {
                if (syscallNr >= sizeof(syscallFunctions) || syscallFunctions[syscallNr] == nullptr) {
                    cout << "invalid syscall number: " << syscallNr << endl;
                    throw BadUserSpace(process);
                } else {
                    KernelInterruptsLock.unlock();

                    /* KernelInterruptsLock could be locked recusively */
                    assert(!KernelInterruptsLock.isLocked());

                    try {
                        result = syscallFunctions[syscallNr](process, arg0, arg1, arg2, arg3, arg4);
                    } catch (BadUserSpace &e) {
                        /* try to core dump in case of BadUserSpace */
                        try {
                            process->addressSpace.coreDump(thread);
                        }  catch (Exception &e) {
                            cout << "CoreDump failed: " << e.description() << endl;
                        }
                        KernelInterruptsLock.lock();
                        throw;
                    } catch (...) {
                        KernelInterruptsLock.lock();
                        throw;
                    }

                    KernelInterruptsLock.lock();
                }
            } catch (Exception &e) {
                action = e.handle();
            }
        } while(action == Exception::Action::RETRY);

        if (action == Exception::Action::CONTINUE) {
            thread->kernelStack->userRegisterState()->setSyscallResult(result);
        }
    }

    assert((action == Exception::Action::CONTINUE && CurrentThread())
           || (action == Exception::Action::SCHEDULE && !CurrentThread()));

    return action;
}

/* Separate function that is called after a syscall leads to a SCHEDULE action. This is needed
 * because assembly code needs to save callee-saved registers inbetween */
extern "C" [[noreturn]] void SyscallScheduleNext() {
    CurrentProcessor()->scheduler.scheduleNext();
}

