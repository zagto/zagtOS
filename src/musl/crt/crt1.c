#include <features.h>
#include "libc.h"

#define START "_start"


int main();
_Noreturn int __libc_start_main(int (*)(), void *);

void _start(void *startupInfo)
{
    __libc_start_main(main, startupInfo);
}
