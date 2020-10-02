#include <EFI.hpp>

using namespace efi;

extern void LoaderMain();
extern "C" void _init();

extern "C" EFI_STATUS EFIAPI efi_main (EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    ImageHandle = imageHandle;
    InitializeLib(imageHandle, systemTable);

    /* Call global constructors */
    _init();

    LoaderMain();

    /* should never reach here */
    while (1);
}
