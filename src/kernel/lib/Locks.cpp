#include <mutex>
#include <system/Processor.hpp>
#include <interrupts/KernelInterruptsLock.hpp>

void mutex::lock() {
    if (CurrentProcessor) {
        assert(!KernelInterruptsLock.isLocked());
    }

    /* TODO: make actually a mutex */
    while (true) {
        size_t expected = 0;
        if (__atomic_compare_exchange_n(&value,
                                        &expected,
                                        1,
                                        false,
                                        __ATOMIC_SEQ_CST,
                                        __ATOMIC_SEQ_CST)) {
            return;
        }
    };

}

bool mutex::trylock() {
    if (CurrentProcessor) {
        assert(!KernelInterruptsLock.isLocked());
    }

    size_t expected = 0;
    return __atomic_compare_exchange_n(&value,
                                      &expected,
                                      1,
                                      false,
                                      __ATOMIC_SEQ_CST,
                                      __ATOMIC_SEQ_CST);
}

void mutex::unlock() {
    if (CurrentProcessor) {
        assert(!KernelInterruptsLock.isLocked());
    }

    size_t expected = 1;
    bool ok = __atomic_compare_exchange_n(&value,
                                         &expected,
                                         0,
                                         false,
                                         __ATOMIC_SEQ_CST,
                                         __ATOMIC_SEQ_CST);
    if (!ok) {
       cout << "Attempt to unlock non-locked lock" << endl;
       Panic();
    }
}

void SpinLock::lock() {
    if (CurrentProcessor) {
        assert(KernelInterruptsLock.isLocked());
    }

    while (true) {
        size_t expected = 0;
        if (__atomic_compare_exchange_n(&value,
                                        &expected,
                                        1,
                                        false,
                                        __ATOMIC_SEQ_CST,
                                        __ATOMIC_SEQ_CST)) {
            return;
        }
    };
}

bool SpinLock::trylock() {
    if (CurrentProcessor) {
        assert(KernelInterruptsLock.isLocked());
    }

    size_t expected = 0;
    return __atomic_compare_exchange_n(&value,
                                      &expected,
                                      1,
                                      false,
                                      __ATOMIC_SEQ_CST,
                                      __ATOMIC_SEQ_CST);
}

void SpinLock::unlock() {
    if (CurrentProcessor) {
        assert(KernelInterruptsLock.isLocked());
    }

    size_t expected = 1;
    bool ok = __atomic_compare_exchange_n(&value,
                                         &expected,
                                         0,
                                         false,
                                         __ATOMIC_SEQ_CST,
                                         __ATOMIC_SEQ_CST);
    if (!ok) {
       cout << "Attempt to unlock non-locked lock" << endl;
       Panic();
    }
}
