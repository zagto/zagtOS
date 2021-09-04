#include <mutex>
#include <system/Processor.hpp>


void mutex::lock() {
    assert(!Processor::kernelInterruptsLock.isLocked());

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
    assert(!Processor::kernelInterruptsLock.isLocked());

    size_t expected = 0;
    return __atomic_compare_exchange_n(&value,
                                      &expected,
                                      1,
                                      false,
                                      __ATOMIC_SEQ_CST,
                                      __ATOMIC_SEQ_CST);
}

void mutex::unlock() {
    assert(!Processor::kernelInterruptsLock.isLocked());

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
    assert(Processor::kernelInterruptsLock.isLocked());

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
    assert(Processor::kernelInterruptsLock.isLocked());

    size_t expected = 0;
    return __atomic_compare_exchange_n(&value,
                                      &expected,
                                      1,
                                      false,
                                      __ATOMIC_SEQ_CST,
                                      __ATOMIC_SEQ_CST);
}

void SpinLock::unlock() {
    assert(Processor::kernelInterruptsLock.isLocked());

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
