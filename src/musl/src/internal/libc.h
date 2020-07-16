#ifndef LIBC_H
#define LIBC_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <zagtos/object.h>

struct __locale_map;

struct __locale_struct {
	const struct __locale_map *cat[6];
};

struct __libc {
	int threaded;
    int secure;
    size_t master_tls_base, tls_size;
	volatile int threads_minus_1;
	struct __locale_struct global_locale;
};

extern hidden struct __libc __libc;
#define libc __libc

hidden void __init_libc(char **envp, char *pn, size_t tls_base, uint32_t main_thread_handle);
hidden void __init_ssp(void);
hidden void __init_tls(size_t, uint32_t);
hidden void __libc_start_init(void);
hidden void __funcs_on_exit(void);
hidden void __funcs_on_quick_exit(void);
hidden void __libc_exit_fini(void);

extern char *__progname, *__progname_full;

extern hidden const char __libc_version[];

hidden void __synccall(void (*)(void *), void *);
hidden int __setxid(int, int, int, int);

#endif
