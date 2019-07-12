#include <stdlib.h>
#include "syscall.h"

_Noreturn void abort(void)
{
    while(1) zagtos_syscall(SYS_EXIT, 127);
}
