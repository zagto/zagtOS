#ifndef LOCK_HPP
#define LOCK_HPP

#include <common/common.hpp>

extern "C" void basicLock(volatile size_t &value);
extern "C" void basicUnlock(volatile size_t &value);

class LockHolder;

class Lock {
private:
    volatile size_t value{0};

public:
    friend class LockHolder;
public:
    void lock() {
        basicLock(value);
    }
    void unlock() {
        if (!value) {
            cout << "Attempt to unlock non-locked lock" << endl;
            Panic();
        }
        basicUnlock(value);
    }

public:
    bool isLocked() {
        return value;
    }
};

class LockHolder {
private:
    Lock *lock;

public:
    LockHolder(Lock &lock) : lock{&lock} {
        lock.lock();
    }
    ~LockHolder() {
        lock->unlock();
    }
};

#endif // LOCK_HPP
