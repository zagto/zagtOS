#pragma once

class KernelInterruptsLockClass {
public:
    KernelInterruptsLockClass() {}
    KernelInterruptsLockClass(const KernelInterruptsLockClass &) = delete;
    void operator=(const KernelInterruptsLockClass &) = delete;

    void lock();
    void unlock();
    bool isLocked() const;
};

extern KernelInterruptsLockClass KernelInterruptsLock;
