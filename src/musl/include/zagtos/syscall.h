#ifndef __ZAGTOS_SYSCALL_H
#define __ZAGTOS_SYSCALL_H

#include <stdint.h>
#include <string.h>

size_t zagtos_syscall();
#define zagtos_uuid_syscall_1_0(call, uuid) zagtos_syscall(call, \
    uuid.data[0] >> 32, \
    uuid.data[0] & 0xffffffff, \
    uuid.data[1] >> 32, \
    uuid.data[1] & 0xffffffff)

#define zagtos_uuid_syscall_1_1(call, uuid, p1) zagtos_syscall(call, \
    uuid.data[0] >> 32, \
    uuid.data[0] & 0xffffffff, \
    uuid.data[1] >> 32, \
    uuid.data[1] & 0xffffffff, \
    p1 )


static const uint32_t SYS_LOG = 1,
                      SYS_EXIT = 2,
                      SYS_SEND_MESSAGE = 3,
                      SYS_WAIT_MESSAGE = 4,
                      SYS_CREATE_PORT = 5,
                      SYS_RANDOM = 6,

                      SYS_MPROTECT = 10,
                      SYS_MMAP = 11,
                      SYS_MUNMAP = 12,
                      SYS_MSYNC = 13,
                      SYS_MREMAP = 14,

                      SYS_CREATE_THREAD = 20,
                      SYS_EXIT_THREAD = 21,
                      SYS_FUTEX = 22,
                      SYS_SET_ROBUST_LIST = 23,
                      SYS_GET_ROBUST_LIST = 24,
                      SYS_YIELD = 25,
                      SYS_SET_THREAD_AREA = 26,
                      SYS_GET_THREAD_AREA = 27,

                      SYS_CLOCK_GETTIME = 30,
                      SYS_CLOCK_NANOSLEEP = 31,

                      SYS_REGISTER_INTERRUPT = 40,
                      SYS_UNREGISTER_INTERRUPT = 41,
                      SYS_GET_ACPI_ROOT = 42,
                      SYS_GET_PHYSICAL_ADDRESS = 43,
                      SYS_IO_PORT_READ = 44,
                      SYS_IO_PORT_WRITE = 45,
                      SYS_ADD_PROCESSOR = 46;

#endif
