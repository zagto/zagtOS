//===-- sanitizer_zagtos.cc -----------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between various sanitizers' runtime libraries and
// implements Zagtos-specific functions.
//===----------------------------------------------------------------------===//

#include "sanitizer_zagtos.h"
#if SANITIZER_ZAGTOS

#include "sanitizer_file.h"
#include "sanitizer_symbolizer.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <zagtos/syscall.h>


extern "C" size_t zagtos_syscall0(size_t call);


namespace __sanitizer {

#include "sanitizer_syscall_generic.inc"

void NORETURN internal__exit(int) {
  zagtos_syscall0(8); // SYS_CRASH
  while (1); // should never go here
}

uptr internal_sched_yield() {
  return 0;
}

void internal_usleep(u64 useconds) {
  struct timespec ts;
  ts.tv_sec = useconds / 1000000;
  ts.tv_nsec = (useconds % 1000000) * 1000;
  zagtos_syscall4(SYS_CLOCK_NANOSLEEP, /*flags*/ 0, CLOCK_MONOTONIC,
                  reinterpret_cast<size_t>(&ts), reinterpret_cast<size_t>(&ts));
}

uptr internal_getpid() {
  return 0;
}

bool FileExists(const char *) {
    return false;
}

uptr GetThreadSelf() { return static_cast<uptr>(pthread_self()); }

tid_t GetTid() { return GetThreadSelf(); }

void Abort() { abort(); }

bool CreateDir(const char *pathname) {
  return false;
}

int Atexit(void (*function)(void)) { return atexit(function); }

void SleepForSeconds(int seconds) { sleep(seconds); }

void SleepForMillis(int millis) { usleep(millis * 1000); }

bool SupportsColoredOutput(fd_t) { return false; }

void GetThreadStackTopAndBottom(bool at_initialization,
                                uptr *stack_top, uptr *stack_bottom) {
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  CHECK_EQ(pthread_getattr_np(pthread_self(), &attr), 0);
  void *base = nullptr;
  size_t size = 0;
  CHECK_EQ(pthread_attr_getstack(&attr, &base, &size), 0);
  CHECK_EQ(pthread_attr_destroy(&attr), 0);

  *stack_bottom = reinterpret_cast<uptr>(base);
  *stack_top = *stack_bottom + size;
}

void GetThreadStackAndTls(bool main, uptr *stk_addr, uptr *stk_size,
                          uptr *tls_addr, uptr *tls_size) {
  uptr stack_top, stack_bottom;
  GetThreadStackTopAndBottom(main, &stack_top, &stack_bottom);
  *stk_addr = stack_bottom;
  *stk_size = stack_top - stack_bottom;
  *tls_addr = *tls_size = 0;
}

void MaybeReexec() {}
void CheckASLR() {}
void DisableCoreDumperIfNecessary() {}
void InstallDeadlySignalHandlers(SignalHandlerType) {}
void SetAlternateSignalStack() {}
void UnsetAlternateSignalStack() {}
void InitTlsSize() {}

void PrintModuleMap() {}

void SignalContext::DumpAllRegisters(void *) {}
const char *DescribeSignalOrException(int) { UNIMPLEMENTED(); }

void FutexWait(atomic_uint32_t *p, u32 cmp) {
  constexpr size_t FUTEX_WAIT = 0;
  size_t status = zagtos_syscall4(SYS_FUTEX, reinterpret_cast<size_t>(p), FUTEX_WAIT, *reinterpret_cast<int32_t *>(p), 0);
  if (status != EAGAIN)  // Normal race.
    CHECK_EQ(status, 0);
}

void FutexWake(atomic_uint32_t *p, u32 count) {
  constexpr size_t FUTEX_WAKE = 1;
  size_t status = zagtos_syscall4(SYS_FUTEX, reinterpret_cast<size_t>(p), FUTEX_WAKE, *reinterpret_cast<int32_t *>(p), 0);
  CHECK_EQ(status, 0);
}

uptr GetPageSize() { return 0x1000; }

uptr GetMmapGranularity() { return GetPageSize(); }

uptr GetMaxVirtualAddress() {
  return (uptr) -1;  // 0xffffffff...
}

void *MmapOrDie(uptr size, const char *mem_type, bool raw_report) {
  void* ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
  if (UNLIKELY(!ptr))
    ReportMmapFailureAndDie(size, mem_type, "allocate", errno, raw_report);
  IncreaseTotalMmap(size);
  return ptr;
}

void *MmapOrDieOnFatalError(uptr size, const char *mem_type) {
  void* ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
  if (UNLIKELY(!ptr)) {
    if (errno == ENOMEM)
      return nullptr;
    ReportMmapFailureAndDie(size, mem_type, "allocate", false);
  }
  IncreaseTotalMmap(size);
  return ptr;
}

void *MmapAlignedOrDieOnFatalError(uptr size, uptr alignment,
                                   const char *mem_type) {
  CHECK(IsPowerOfTwo(size));
  CHECK(IsPowerOfTwo(alignment));
  CHECK(alignment <= GetPageSize());
  void* ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
  if (UNLIKELY(!ptr))
    ReportMmapFailureAndDie(size, mem_type, "align allocate", errno, false);
  IncreaseTotalMmap(size);
  return ptr;
}

void *MmapNoReserveOrDie(uptr size, const char *mem_type) {
  return MmapOrDie(size, mem_type, false);
}

void UnmapOrDie(void *addr, uptr size) {
  if (!addr || !size) return;
  munmap(addr, size);
  DecreaseTotalMmap(size);
}

fd_t OpenFile(const char *, FileAccessMode, error_t *error_p) {
    *error_p = ENOTSUP;
    return kInvalidFd;
}

bool ReadFromFile(fd_t, void *, uptr, uptr *, error_t *error_p) {
    *error_p = ENOTSUP;
    return false;
}

bool WriteToFile(fd_t, const void *, uptr, uptr *, error_t *error_p) {
    *error_p = ENOTSUP;
    return false;
}


void CloseFile(fd_t) {}

void ReleaseMemoryPagesToOS(uptr beg, uptr end) {}
void DumpProcessMap() {}

bool IsAccessibleMemoryRange(uptr beg, uptr size) {
  return true;
}

char **GetArgv() { return nullptr; }

const char *GetEnv(const char *name) {
  return getenv(name);
}

uptr ReadBinaryName(/*out*/char *buf, uptr buf_len) {
  internal_strncpy(buf, "StubBinaryName", buf_len);
  return internal_strlen(buf);
}

uptr ReadLongProcessName(/*out*/ char *buf, uptr buf_len) {
  internal_strncpy(buf, "StubProcessName", buf_len);
  return internal_strlen(buf);
}

bool IsPathSeparator(const char) {
  return false;
}

bool IsAbsolutePath(const char *) {
  return false;
}

void ReportFile::Write(const char *buffer, uptr length) {
  zagtos_syscall2(SYS_LOG, reinterpret_cast<size_t>(buffer), length);
}

void ListOfModules::init() {
    clear();
}

void ListOfModules::fallbackInit() {
    clear();
}

void InitializePlatformEarly() {}

void InitializePlatformCommonFlags(CommonFlags *cf) {}

} // namespace __sanitizer

#endif  // SANITIZER_ZAGTOS
