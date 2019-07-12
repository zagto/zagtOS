#ifndef UTIL_H
#define UTIL_H

#include <efi.h>

#define PAGE_SIZE 0x1000

__attribute__((noreturn)) void Halt(void);

void Log(const char *message);
void Log16(const CHAR16 *message);
void LogStatus(const char *message, EFI_STATUS status);
void LogUINTN(UINTN value);

// This function should be called to make log no longer rely on boot services
void LogExitBootServices(void);

#endif // UTIL_H
