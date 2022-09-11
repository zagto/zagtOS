#ifndef __ZAGTOS_SYSCALL_H
#define __ZAGTOS_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t zagtos_syscall();

size_t zagtos_syscall0(size_t call);
size_t zagtos_syscall1(size_t call, size_t a);
size_t zagtos_syscall2(size_t call, size_t a, size_t b);
size_t zagtos_syscall3(size_t call, size_t a, size_t b, size_t c);
size_t zagtos_syscall4(size_t call, size_t a, size_t b, size_t c, size_t d);
size_t zagtos_syscall5(size_t call, size_t a, size_t b, size_t c, size_t d, size_t e);

static const uint32_t SYS_LOG = 1,
                      SYS_EXIT = 2,
                      SYS_SEND_MESSAGE = 3,
                      SYS_WAIT_FOR_EVENT = 4,
                      SYS_CREATE_PORT = 5,
                      SYS_DELETE_HANDLE = 6,
                      SYS_RANDOM = 7,
                      SYS_CRASH = 8,
                      SYS_CREATE_EVENT_QUEUE = 9,

                      SYS_MPROTECT = 10,
                      SYS_MMAP = 11,
                      SYS_MUNMAP = 12,
                      SYS_MSYNC = 13,
                      SYS_MREMAP = 14,
                      SYS_CREATE_SHARED_MEMORY = 15,

                      SYS_CREATE_THREAD = 20,
                      SYS_EXIT_THREAD = 21,
                      SYS_FUTEX = 22,
                      SYS_YIELD = 23,

                      SYS_CLOCK_GETTIME = 30,
                      SYS_CLOCK_NANOSLEEP = 31,

                      SYS_CREATE_IO_PORT_RANGE = 41,
                      SYS_GET_ACPI_ROOT = 42,
                      SYS_IO_PORT_READ = 43,
                      SYS_IO_PORT_WRITE = 44,
                      SYS_CREATE_INTERRUPT = 45,
                      SYS_SUBSCRIBE_INTERRUPT = 46,
                      SYS_UNSUBSCRIBE_INTERRUPT = 47,
                      SYS_PROCESSED_INTERRUPT = 48,
                      SYS_WAIT_INTERRUPT = 49,

                      SYS_SPAWN_PROCESS = 50,

                      SYS_PIN_THREAD = 60;

#ifdef __cplusplus
}
#endif

#endif
