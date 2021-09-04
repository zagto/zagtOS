#include <common/common.hpp>
#include <memory>
#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <syscalls/UserSpaceObject.hpp>
#include <processes/FutexManager.hpp>
#include <system/System.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <system/Processor.hpp>


static constexpr uint32_t FUTEX_WAIT = 0,
                          FUTEX_WAKE = 1,
                          FUTEX_FD = 2,
                          FUTEX_LOCK_PI = 6,
                          FUTEX_UNLOCK_PI = 7,
                          FUTEX_PRIVATE = 128;

static constexpr uint32_t FUTEX_HANDLE_MASK = 0x4fffffff;
static constexpr uint32_t FUTEX_WAITERS_BIT = 0x80000000;


Result<size_t> Futex(const shared_ptr<Process> &process,
                     size_t address,
                     size_t operation,
                     size_t passedValue,
                     size_t timeoutOrValue2,
                     size_t) {
    cout << "Futex(" << address << ", " << operation <<", "<< timeoutOrValue2 << ", " << passedValue << ")" << endl;

    bool isPrivate = operation & FUTEX_PRIVATE;
    operation = operation & ~FUTEX_PRIVATE;
    FutexManager &manager = isPrivate ? CurrentSystem.futexManager : process->futexManager;

    scoped_lock sl2(manager.lock);

    timespec timeout{0, 0};

    if (operation == FUTEX_WAIT || operation == FUTEX_LOCK_PI) {
        if (timeoutOrValue2 != 0) {
            Status status = Status::OK();
            UserSpaceObject<timespec, USOOperation::READ> timeoutUSO(timeoutOrValue2, status);
            if (!status) {
                if (status == Status::BadUserSpace()) {
                    cout << "Futex: invalid pointer to timeout: " << timeoutOrValue2 << endl;
                }
                return status;
            }
        }
    }

    if (timeout.tv_sec != 0 || timeout.tv_nsec != 0) {
        cout << "TODO: handle timeout" << endl;
        return Status::BadUserSpace();
    }

    // TODO: actually do the priority thing

    Result futexIdResult = process->addressSpace.getFutexID(address);
    if (!futexIdResult) {
        return futexIdResult.status();
    }
    size_t futexID = *futexIdResult;

    switch (operation) {
    case FUTEX_WAIT: {
        /* read value back - otherwise a wake may have occured inbetween and we wait forever */
        Result<uint32_t> directValue = process->addressSpace.atomicCopyFrom32(address);
        if (!directValue) {
            return directValue.status();
        }
        if (*directValue != passedValue) {
            return EAGAIN;
        }
        Status status = manager.wait(futexID);
        /* Wait will return DiscardStateAndSchedule on success */
        assert(!status);
        return status;
    }
    case FUTEX_WAKE: {
        size_t numWoken = manager.wake(futexID, passedValue);
        return numWoken;
    }
    case FUTEX_LOCK_PI:
        while (true) {
            Result<uint32_t> copyResult = process->addressSpace.atomicCopyFrom32(address);
            if (!copyResult) {
                return copyResult.status();
            }

            if (*copyResult == 0) {
                Result<bool> exchangeResult = process->addressSpace.atomicCompareExchange32(
                            address, 0, CurrentThread()->handle());
                if (!exchangeResult) {
                    return exchangeResult.status();
                }

                if (*exchangeResult) {
                    return 0;
                }
            } else {
                Result<bool> exchangeResult = process->addressSpace.atomicCompareExchange32(
                             address, *copyResult, *copyResult | FUTEX_WAITERS_BIT);
                if (!exchangeResult) {
                    return exchangeResult.status();
                }

                if (*exchangeResult) {
                    Status status = manager.wait(futexID);
                    /* Wait will return DiscardStateAndSchedule on success */
                    assert(!status);

                    /* we leave the FUTEX_WAITERS_BIT set here. This is safe as even if the
                     * futex is unlocked before we retry, the unlocking thread will do a
                     * syscall unnecessarily but not causing any problems. */
                    return status;
                }
            }
        }
    case FUTEX_UNLOCK_PI: {
        Result<uint32_t> copyResult = process->addressSpace.atomicCopyFrom32(address);
        if (!copyResult) {
            return copyResult.status();
        }

        if ((*copyResult & FUTEX_HANDLE_MASK) != CurrentThread()->handle()) {
            cout << "Attempt to unlock Priority-inheritance futex from wrong thread" << endl;
            return EPERM;
        }
        manager.wake(futexID, 1);
        return 0;
    }
    default:
        cout << "Futex: unsupported operation " << operation << endl;
        return Status::BadUserSpace();
    }
}
