#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "syscall.h"
#include "atomic.h"
#include "libc.h"
#include <zagtos/Messaging.h>
#include <zagtos/unixcompat.h>

extern weak hidden void (*const __init_array_start)(void), (*const __init_array_end)(void);

extern struct ZoMessageInfo *__run_message;

static char *dummy_environ = NULL;
static char *dummy_pn = "/run";

static ZFileDescriptor syslog_fd = {
    .port_handle = -1,
    .position = 0,
    .is_syslog = 1,
    .read = 1,
    .write = 1,
    .lock = 0,
    .refcount = 4, /* never try to deallocate */
    .flags = O_RDWR,
};

weak void _init();


#ifdef __GNUC__
__attribute__((__noinline__))
#endif
void __init_libc(char **envp, char *pn, size_t tls_pointer)

{
    __environ = envp;
    __progname = pn;
    __progname_full = pn;
    int i;
    for (i=0; pn[i]; i++) if (pn[i]=='/') __progname = pn+i+1;

    __init_tls(tls_pointer, 1);
    __init_ssp();

    libc.secure = 1;
}

static void libc_start_init(void)
{
	_init();
	uintptr_t a = (uintptr_t)&__init_array_start;
	for (; a<(uintptr_t)&__init_array_end; a+=sizeof(void(*)()))
		(*(void (**)(void))a)();
}

weak_alias(libc_start_init, __libc_start_init);

struct TLSInfo {
    uint64_t masterTLSStart;
    uint64_t masterTLSLength;
    uint64_t threadTLSPointer;
};

typedef int lsm2_fn(int (*)(int,char **,char **), int, char **);
static lsm2_fn libc_start_main_stage2;

int __libc_start_main(int (*main)(int,char **,char **),
                      struct ZoMessageInfo *run_msg)
{
    int argc;
    char **argv;
    char **envp;

    __run_message = run_msg;

    if (!uuid_compare(run_msg->type, ZAGTOS_MSG_UNIX_RUN)) {
        ZUnixRun *urun = (ZUnixRun *)run_msg;
        argc = urun->argc;
        argv = calloc(argc, sizeof(char *));
        assert(argv);

        char *p = &urun->argv_environ_blob;
        int arg_index = 0;
        char *begin = p;
        while (arg_index < argc) {
            if (!*p) {
                argv[arg_index] = begin;
                arg_index++;
                begin = p + 1;
            }
            p++;
        }

        envp = calloc(urun->environ_count + 1, sizeof(char *));
        int env_index = 0;
        while (arg_index < urun->environ_count) {
            if (!*p) {
                envp[env_index] = begin;
                env_index++;
                begin = p + 1;
            }
            p++;
        }
        envp[urun->environ_count] = NULL;

        // TODO: stdin/out
    } else {
        argc = 1;
        argv = &dummy_pn;
        envp = &dummy_environ;
        zagtos_init_file_descriptor(0, &syslog_fd);
        zagtos_init_file_descriptor(1, &syslog_fd);
        zagtos_init_file_descriptor(2, &syslog_fd);
    }

    struct TLSInfo *tlsInfo = (struct TLSInfo *)0x7FFFFFDF5800;

	/* External linkage, and explicit noinline attribute if available,
	 * are used to prevent the stack frame used during init from
	 * persisting for the entire process lifetime. */
    libc.master_tls_start = tlsInfo->masterTLSStart;
    libc.tls_size = tlsInfo->masterTLSLength;
    __init_libc(envp, argv[0], tlsInfo->threadTLSPointer);

	/* Barrier against hoisting application code or anything using ssp
	 * or thread pointer prior to its initialization above. */
	lsm2_fn *stage2 = libc_start_main_stage2;
	__asm__ ( "" : "+r"(stage2) : : "memory" );
	return stage2(main, argc, argv);
}

static int libc_start_main_stage2(int (*main)(int,char **,char **), int argc, char **argv)
{
	char **envp = argv+argc+1;
	__libc_start_init();

	/* Pass control to the application */
	exit(main(argc, argv, envp));
	return 0;
}
