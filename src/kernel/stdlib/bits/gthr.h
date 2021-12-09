#pragma once

//#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t __gthread_mutex_t;
typedef size_t __gthread_recursive_mutex_t;
typedef size_t __gthread_once_t;
typedef size_t __gthread_key_t;

#define __GTHREAD_MUTEX_INIT 0;
#define __GTHREAD_ONCE_INIT 0;

int __gthread_once (__gthread_once_t *once, void (*func) ());

int __gthread_key_create (__gthread_key_t *keyp, void (*dtor) (void *));
int __gthread_key_delete (__gthread_key_t key);

void *__gthread_getspecific (__gthread_key_t key);
int __gthread_setspecific (__gthread_key_t key, const void *ptr);

int __gthread_mutex_lock (__gthread_mutex_t *mutex);
int __gthread_mutex_unlock (__gthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif
