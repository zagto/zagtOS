#include <stdlib.h>
#include <zagtos/syscall.h>

_Noreturn void _Exit(int ec)
{
    zagtos_syscall(SYS_EXIT, ec);
    for (;;) /* should never reach */;
}
