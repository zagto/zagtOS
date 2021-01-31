#include <common/common.hpp>
#include <memory>
#include <lib/atomic.hpp>
#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/UserSpaceObject.hpp>
#include <processes/FutexManager.hpp>
#include <system/System.hpp>
#include <syscalls/ErrorCodes.hpp>


static constexpr uint32_t FUTEX_WAIT = 0,
                          FUTEX_WAKE = 1,
                          FUTEX_FD = 2,
                          FUTEX_LOCK_PI = 6,
                          FUTEX_UNLOCK_PI = 7,
                          FUTEX_PRIVATE = 128;

static constexpr uint32_t FUTEX_HANDLE_MASK = 0x4fffffff;
static constexpr uint32_t FUTEX_WAITERS_BIT = 0x80000000;


size_t Futex(Thread *thread, size_t address, uint32_t operation, size_t timeoutOrValue2, int32_t passedValue) {
    bool isPrivate = operation & FUTEX_PRIVATE;
    operation = operation & ~FUTEX_PRIVATE;
    FutexManager &manager = isPrivate ? CurrentSystem.futexManager : thread->process->futexManager;

    scoped_lock sl1(thread->process->pagingLock);
    scoped_lock sl2(manager.lock);

    if (address % 4 != 0 || !thread->process->verifyUserAccess(address, 4, true)) {
        cout << "Futex: invalid address" << endl;
        Panic(); // TODO: exception
    }

    auto physicalAddress = thread->process->pagingContext->resolve(UserVirtualAddress(address));
    volatile int32_t *directValue = physicalAddress.identityMapped().asPointer<volatile int32_t>();

    timespec timeout{0, 0};

    if (operation == FUTEX_WAIT || operation == FUTEX_LOCK_PI) {
        if (timeoutOrValue2 != 0) {
            UserSpaceObject<timespec, USOOperation::READ> timeoutUSO(timeoutOrValue2, thread->process);
            if (!timeoutUSO.valid) {
                cout << "Futex: invalid pointer to timeout" << endl;
                Panic(); // TODO: exception
            }
        }
    }

    if (timeout.tv_sec != 0 || timeout.tv_nsec != 0) {
        cout << "TODO: handle timeout" << endl;
        Panic();
    }

    // TODO: actually do the priority thing

    switch (operation) {
    case FUTEX_WAIT:
        /* read value back - otherwise a wake may have occured inbetween and we wait forever */
        if (*directValue != passedValue) {
            return EAGAIN;
        }
        manager.wait(physicalAddress, thread);
        return 0;
    case FUTEX_WAKE: {
        size_t numWoken = manager.wake(physicalAddress, passedValue);
        return numWoken;
    }
    case FUTEX_LOCK_PI:
        while (true) {
            int32_t value = *directValue;
            if (value == 0) {
                if (compare_exchange_i32(*directValue, value, thread->handle())) {
                    return 0;
                }
            } else {
                if (compare_exchange_i32(*directValue, value, value | FUTEX_WAITERS_BIT)) {
                    manager.wait(physicalAddress, thread);
                    return 0;
                }
            }
        }
    case FUTEX_UNLOCK_PI:
        if ((*directValue & FUTEX_HANDLE_MASK) != thread->handle()) {
            cout << "Attempt to unlock Priority-inheritance futex from wrong thread" << endl;
            return EPERM;
        }
        manager.wake(physicalAddress, 1);
        return 0;
    default:
        cout << "Futex: unsupported operation " << operation << endl;
        Panic(); // TODO: exception
    }
}
