#include <threads.h>
#include "syscall.h"

void thrd_yield()
{
    zagtos_syscall(SYS_YIELD);
}
