#include "pthread_impl.h"
#include "atomic.h"
#include "syscall.h"
/* cheat and reuse CRTJMP macro from dynlink code */

static void *unmap_base;
static size_t unmap_size;
static char shared_stack[256];

void __unmapself(void *base, size_t size)
{
	char *stack = shared_stack + sizeof shared_stack;
	stack -= (uintptr_t)stack % 16;
	unmap_base = base;
	unmap_size = size;
    zagtos_syscall(SYS_EXIT);
}
