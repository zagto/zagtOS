#include <limits.h>
#include <sys/mman.h>
#include <string.h>
#include <stddef.h>
#include "pthread_impl.h"
#include "libc.h"
#include "atomic.h"
#include "syscall.h"

volatile int __thread_list_lock;

#define MIN_TLS_ALIGN offsetof(struct builtin_tls, pt)

static void static_init_tls(size_t thread_area_addr)
{
    pthread_t pt = (pthread_t)(thread_area_addr - pthread_struct_area_size);
    pt->self = pt;
    pt->detach_state = DT_JOINABLE;
    pt->tid = 0;
    pt->locale = &libc.global_locale;
    pt->robust_list.head = &pt->robust_list.head;
    pt->next = pt->prev = pt;
}

weak_alias(static_init_tls, __init_tls);
