#pragma once

namespace efi {

extern "C" {
    #include <efi.h>
    #include <efilib.h>
}

#undef NULL

extern EFI_HANDLE ImageHandle;
extern EFI_SYSTEM_TABLE *SystemTable;

const char *statusToString(EFI_STATUS status);

}
