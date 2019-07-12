#include <features.h>
#include "libc.h"

#define START "_start"


int main();
_Noreturn int __libc_start_main(int (*)(), void *, size_t, size_t, size_t);

void _start(void *msg, size_t tls_base, size_t master_tls_base, size_t tls_size)
{
	int argc = 0;
	char *av = 0;
	char **argv = &av;
    __libc_start_main(main, msg, tls_base, master_tls_base, tls_size);
}
