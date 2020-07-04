#include <common/common.hpp>
#include <memory>
#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/UserSpaceObject.hpp>
#include <processes/FutexManager.hpp>
#include <system/System.hpp>
#include <syscalls/ErrorCodes.hpp>


static constexpr uint32_t FUTEX_WAIT = 0,
                          FUTEX_WAKE = 1,
                          FUTEX_FD = 2,
                          FUTEX_PRIVATE = 128;


bool Futex(Thread *thread, RegisterState &registerState) {
    size_t address = registerState.syscallParameter(0);
    uint32_t operation = registerState.syscallParameter(1);
    UserSpaceObject<timespec, USOOperation::READ> timeout(registerState.syscallParameter(2),
                                                          thread->process);
    int32_t passedValue = registerState.syscallParameter(3);

    registerState.setSyscallResult(0);

    bool isPrivate = operation & FUTEX_PRIVATE;
    operation = operation & ~FUTEX_PRIVATE;
    FutexManager &manager = isPrivate ? CurrentSystem.futexManager : thread->process->futexManager;

    scoped_lock sl1(thread->process->pagingLock);
    scoped_lock sl2(manager.lock);

    if (address % 4 != 0 || !thread->process->verifyUserAccess(address, 4, true)) {
        cout << "Futex: invalid address" << endl;
        return false;
    }

    auto physicalAddress = thread->process->pagingContext->resolve(UserVirtualAddress(address));
    volatile int32_t *directValue = physicalAddress.identityMapped().asPointer<volatile int32_t>();

    switch (operation) {
    case FUTEX_WAIT:
        /* read value back - otherwise a wake may have occured inbetween and we wait forever */
        if (*directValue != passedValue) {
            registerState.setSyscallResult(EAGAIN);
            return true;
        }
        manager.wait(physicalAddress, thread);
        return true;
    case FUTEX_WAKE:
        return manager.wake(physicalAddress, passedValue);
    default:
        cout << "Futex: unsupported operation " << operation << endl;
        return false;
    }
}
