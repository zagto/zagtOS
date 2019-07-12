#define _GNU_SOURCE
#include "pthread_impl.h"
#include <sys/mman.h>

int pthread_getattr_np(pthread_t t, pthread_attr_t *a)
{
	*a = (pthread_attr_t){0};
	a->_a_detach = t->detach_state>=DT_DETACHED;
	a->_a_guardsize = t->guard_size;
	if (t->stack) {
		a->_a_stackaddr = (uintptr_t)t->stack;
		a->_a_stacksize = t->stack_size;
	} else {
        a->_a_stackaddr = 0x7FFFFFE00000;
        a->_a_stacksize = 0x200000;
    }
	return 0;
}
