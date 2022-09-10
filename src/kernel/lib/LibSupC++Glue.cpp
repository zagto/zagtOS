#include <memory/DLMallocGlue.hpp>
#include <iostream>
#include <bits/gthr.h>
#include <mutex>
#include <interrupts/KernelInterruptsLock.hpp>

extern "C" void free(void *pointer) {
    DLMallocGlue.free(KernelVirtualAddress(pointer));
}

extern "C" int strcmp(const char *s1, const char *s2) {
    size_t position = 0;
    while (true) {
        if (s1[position] < s2[position]) {
            return -1;
        } else if (s1[position] > s2[position]) {
            return 1;
        } else if (s1[position] == 0) {
            return 0;
        }
        position++;
    }
}

extern "C" void abort() {
    cout << "abort() called by libsupc++" << endl;
    Panic();
}

extern "C" size_t strlen(const char *s) {
    size_t position = 0;
    while (s[position] != 0) {
        position++;
    }
    return position;
}

int __gthread_once (__gthread_once_t *once, void (*func) ());

int __gthread_key_create (__gthread_key_t *keyp, void (*dtor) (void *));
int __gthread_key_delete (__gthread_key_t key);

void *__gthread_getspecific (__gthread_key_t key);
int __gthread_setspecific (__gthread_key_t key, const void *ptr);

/* gthread mutexes are actually spinlocks */
extern "C" int __gthread_mutex_lock (__gthread_mutex_t *_mutex) {
    KernelInterruptsLock.lock();
    reinterpret_cast<SpinLock *>(_mutex)->lock();
    return 0;
}

extern "C" int __gthread_mutex_unlock (__gthread_mutex_t *_mutex) {
    reinterpret_cast<SpinLock *>(_mutex)->unlock();
    KernelInterruptsLock.unlock();
    return 0;
}
