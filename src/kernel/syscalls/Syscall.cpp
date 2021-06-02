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
#include <syscalls/GetFirmwareRoot.hpp>
#include <syscalls/IOPortRead.hpp>
#include <syscalls/IOPortWrite.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#include <interrupts/Interrupts.hpp> // dealWithException

typedef Result<size_t> SyscallFunction(const shared_ptr<Process> &, size_t, size_t, size_t, size_t, size_t);

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
    nullptr, /* RegisterInterrupt */
    nullptr, /* UnregisterInterrupt */
    &GetFirmwareRoot,
    &IOPortRead,
    &IOPortWrite,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    /* 50 */
    &SpawnProcess,
};

extern "C" [[noreturn]]
void Syscall(size_t syscallNr,
             size_t arg0,
             size_t arg1,
             size_t arg2,
             size_t arg3,
             size_t arg4) {
    const shared_ptr<Process> process = CurrentProcess();

    Status status = Status::OK();
    do {
        if (syscallNr >= sizeof(syscallFunctions) || syscallFunctions[syscallNr] == nullptr) {
            cout << "invalid syscall number: " << syscallNr << endl;
            status = Status::BadUserSpace();
        } else {
            Result<size_t> result = syscallFunctions[syscallNr](process, arg0, arg1, arg2, arg3, arg4);
            if (result) {
                CurrentThread()->registerState.setSyscallResult(*result);
            } else {
                status = result.status();
            }
        }

        if (!status) {
            dealWithException(status);
        }
    } while(!status);

    CurrentProcessor->returnToUserMode();
}
