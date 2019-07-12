#define _GNU_SOURCE
#include "pthread_impl.h"
#include "stdio_impl.h"
#include "libc.h"
#include "lock.h"
#include <sys/mman.h>
#include <string.h>
#include <stddef.h>

static int tl_lock_count;
static int tl_lock_waiters;

void __tl_lock(void)
{
	int tid = __pthread_self()->tid;
	int val = __thread_list_lock;
	if (val == tid) {
		tl_lock_count++;
		return;
	}
	while ((val = a_cas(&__thread_list_lock, 0, tid)))
		__wait(&__thread_list_lock, &tl_lock_waiters, val, 0);
}

void __tl_unlock(void)
{
	if (tl_lock_count) {
		tl_lock_count--;
		return;
	}
	a_store(&__thread_list_lock, 0);
	if (tl_lock_waiters) __wake(&__thread_list_lock, 1, 0);
}

void __tl_sync(pthread_t td)
{
	a_barrier();
	int val = __thread_list_lock;
	if (!val) return;
	__wait(&__thread_list_lock, &tl_lock_waiters, val, 0);
	if (tl_lock_waiters) __wake(&__thread_list_lock, 1, 0);
}

_Noreturn void __pthread_exit(void *result)
{
	pthread_t self = __pthread_self();
	sigset_t set;

	self->canceldisable = 1;
	self->cancelasync = 0;
	self->result = result;

	while (self->cancelbuf) {
		void (*f)(void *) = self->cancelbuf->__f;
		void *x = self->cancelbuf->__x;
		self->cancelbuf = self->cancelbuf->__next;
		f(x);
	}

	__pthread_tsd_run_dtors();

	/* Access to target the exiting thread with syscalls that use
	 * its kernel tid is controlled by killlock. For detached threads,
	 * any use past this point would have undefined behavior, but for
	 * joinable threads it's a valid usage that must be handled. */
	LOCK(self->killlock);

    /* The thread list lock must be AS-safe, and thus requires
     * application signals to be blocked before it can be taken. */
    __tl_lock();

	/* If this is the only thread in the list, don't proceed with
	 * termination of the thread, but restore the previous lock and
	 * signal state to prepare for exit to call atexit handlers. */
	if (self->next == self) {
		__tl_unlock();
        UNLOCK(self->killlock);
		exit(0);
	}

	/* At this point we are committed to thread termination. Unlink
	 * the thread from the list. This change will not be visible
	 * until the lock is released, which only happens after SYS_exit
	 * has been called, via the exit futex address pointing at the lock. */
	libc.threads_minus_1--;
	self->next->prev = self->prev;
	self->prev->next = self->next;
	self->prev = self->next = self;

    /* Process robust list in userspace to handle non-pshared mutexes
     * and the detached thread case where the robust list head will
     * be invalid when the kernel would process it. */
    __vm_lock();
    volatile void *volatile *rp;
    while ((rp=self->robust_list.head) && rp != &self->robust_list.head) {
        pthread_mutex_t *m = (void *)((char *)rp
            - offsetof(pthread_mutex_t, _m_next));
        int waiters = m->_m_waiters;
        int priv = (m->_m_type & 128) ^ 128;
        self->robust_list.pending = rp;
        self->robust_list.head = *rp;
        int cont = a_swap(&m->_m_lock, 0x40000000);
        self->robust_list.pending = 0;
        if (cont < 0 || waiters)
            __wake(&m->_m_lock, 1, priv);
    }
    __vm_unlock();

    __do_orphaned_stdio_locks();

	/* This atomic potentially competes with a concurrent pthread_detach
	 * call; the loser is responsible for freeing thread resources. */
	int state = a_cas(&self->detach_state, DT_JOINABLE, DT_EXITING);

	if (state==DT_DETACHED && self->map_base) {
		/* Detached threads must block even implementation-internal
		 * signals, since they will not have a stack in their last
		 * moments of existence. */

        /* Robust list will no longer be valid, and was already
         * processed above, so unregister it with the kernel. */
        if (self->robust_list.off)
            zagtos_syscall(SYS_SET_ROBUST_LIST, 0, 3*sizeof(long));

        /* Since __unmapself bypasses the normal munmap code path,
		 * explicitly wait for vmlock holders first. */
		__vm_wait();

		/* The following call unmaps the thread's stack mapping
		 * and then exits without touching the stack. */
		__unmapself(self->map_base, self->map_size);
	}

	/* Wake any joiner. */
	__wake(&self->detach_state, 1, 1);

	/* After the kernel thread exits, its tid may be reused. Clear it
	 * to prevent inadvertent use and inform functions that would use
	 * it that it's no longer available. */
    self->tid = 0;
	UNLOCK(self->killlock);

    for (;;) zagtos_syscall(SYS_EXIT, 0);
}

void __do_cleanup_push(struct __ptcb *cb)
{
	struct pthread *self = __pthread_self();
	cb->__next = self->cancelbuf;
	self->cancelbuf = cb;
}

void __do_cleanup_pop(struct __ptcb *cb)
{
	__pthread_self()->cancelbuf = cb->__next;
}

struct start_args {
	void *(*start_func)(void *);
	void *start_arg;
	pthread_attr_t *attr;
	volatile int *perr;
};

static int start(void *p)
{
	struct start_args *args = p;
    __pthread_exit(args->start_func(args->start_arg));
	return 0;
}

static int start_c11(void *p)
{
	struct start_args *args = p;
	int (*start)(void*) = (int(*)(void*)) args->start_func;
	__pthread_exit((void *)(uintptr_t)start(args->start_arg));
	return 0;
}

#define ROUND(x) (((x)+PAGE_SIZE-1)&-PAGE_SIZE)

/* pthread_key_create.c overrides this */
static volatile size_t dummy = 0;
weak_alias(dummy, __pthread_tsd_size);
static void *dummy_tsd[1] = { 0 };
weak_alias(dummy_tsd, __pthread_tsd_main);

static FILE *volatile dummy_file = 0;
weak_alias(dummy_file, __stdin_used);
weak_alias(dummy_file, __stdout_used);
weak_alias(dummy_file, __stderr_used);

static void init_file_lock(FILE *f)
{
	if (f && f->lock<0) f->lock = 0;
}

int __pthread_create(pthread_t *restrict res, const pthread_attr_t *restrict attrp, void *(*entry)(void *), void *restrict arg)
{
	int ret, c11 = (attrp == __ATTRP_C11_THREAD);
	size_t size, guard;
	struct pthread *self, *new;
	unsigned char *map = 0, *stack = 0, *tsd = 0, *stack_limit;
	pthread_attr_t attr = { 0 };
	sigset_t set;
	volatile int err = -1;

	self = __pthread_self();
	if (!libc.threaded) {
		for (FILE *f=*__ofl_lock(); f; f=f->next)
			init_file_lock(f);
		__ofl_unlock();
		init_file_lock(__stdin_used);
		init_file_lock(__stdout_used);
		init_file_lock(__stderr_used);
		self->tsd = (void **)__pthread_tsd_main;
		libc.threaded = 1;
	}
	if (attrp && !c11) attr = *attrp;

	__acquire_ptc();
	if (!attrp || c11) {
		attr._a_stacksize = __default_stacksize;
		attr._a_guardsize = __default_guardsize;
	}

	if (attr._a_stackaddr) {
        size_t need = libc.tls_size + pthread_struct_area_size + __pthread_tsd_size;
		size = attr._a_stacksize;
		stack = (void *)(attr._a_stackaddr & -16);
		stack_limit = (void *)(attr._a_stackaddr - size);
		/* Use application-provided stack for TLS only when
		 * it does not take more than ~12% or 2k of the
		 * application's stack space. */
		if (need < size/8 && need < 2048) {
			tsd = stack - __pthread_tsd_size;
            stack = tsd - libc.tls_size - pthread_struct_area_size;
			memset(stack, 0, need);
		} else {
			size = ROUND(need);
		}
		guard = 0;
	} else {
		guard = ROUND(attr._a_guardsize);
		size = guard + ROUND(attr._a_stacksize
            + libc.tls_size + pthread_struct_area_size +  __pthread_tsd_size);
	}

	if (!tsd) {
		if (guard) {
			map = __mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
			if (map == MAP_FAILED) goto fail;
			if (__mprotect(map+guard, size-guard, PROT_READ|PROT_WRITE)
			    && errno != ENOSYS) {
				__munmap(map, size);
				goto fail;
			}
		} else {
			map = __mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
			if (map == MAP_FAILED) goto fail;
		}
		tsd = map + size - __pthread_tsd_size;
		if (!stack) {
            stack = tsd - libc.tls_size - pthread_struct_area_size;
			stack_limit = map + guard;
		}
	}

    new = (struct pthread *)(tsd - libc.tls_size - pthread_struct_area_size);
    memcpy((char *)new + pthread_struct_area_size, (void *)libc.master_tls_base, libc.tls_size);

	new->map_base = map;
	new->map_size = size;
	new->stack = stack;
	new->stack_size = stack - stack_limit;
	new->guard_size = guard;
	new->self = new;
	new->tsd = (void *)tsd;
	new->locale = &libc.global_locale;
	if (attr._a_detach) {
		new->detach_state = DT_DETACHED;
	} else {
		new->detach_state = DT_JOINABLE;
	}
	new->robust_list.head = &new->robust_list.head;
	new->CANARY = self->CANARY;

	/* Setup argument structure for the new thread on its stack.
	 * It's safe to access from the caller only until the thread
	 * list is unlocked. */
	stack -= (uintptr_t)stack % sizeof(uintptr_t);
	stack -= sizeof(struct start_args);
	struct start_args *args = (void *)stack;
	args->start_func = entry;
	args->start_arg = arg;
	if (attr._a_sched) {
		args->attr = &attr;
		args->perr = &err;
	} else {
		args->attr = 0;
		args->perr = 0;
	}

	__tl_lock();
	libc.threads_minus_1++;
    ret = zagtos_syscall(SYS_CREATE_THREAD, c11 ? start_c11 : start, stack, args, new, &new->tid, &__thread_list_lock);

	/* If clone succeeded, new thread must be linked on the thread
	 * list before unlocking it, even if scheduling may still fail. */
	if (ret >= 0) {
		new->next = self->next;
		new->prev = self;
		new->next->prev = new;
		new->prev->next = new;
	}
	__tl_unlock();
	__release_ptc();

	if (ret < 0) {
		libc.threads_minus_1--;
		if (map) __munmap(map, size);
		return EAGAIN;
	}

	if (attr._a_sched) {
		if (a_cas(&err, -1, -2)==-1)
			__wait(&err, 0, -2, 1);
		ret = err;
		if (ret) return ret;
	}

	*res = new;
	return 0;
fail:
	__release_ptc();
	return EAGAIN;
}

weak_alias(__pthread_exit, pthread_exit);
weak_alias(__pthread_create, pthread_create);
