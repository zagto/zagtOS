#ifndef FILE_H
#define FILE_H

#include <efi.h>

EFI_FILE_PROTOCOL *FindOwnVolume(EFI_HANDLE imageHandle);
void *LoadFile(EFI_FILE_PROTOCOL *volume, const CHAR16 *fileName, UINTN *fileSize);

#endif // FILE_H
