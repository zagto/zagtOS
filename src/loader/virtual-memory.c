#include <efi.h>
#include <framestack.h>
#include <physical-memory.h>
#include <virtual-memory.h>
#include <paging.h>


void InitVirtualMemory() {
    MapAddress(SystemInfo, (EFI_PHYSICAL_ADDRESS)AllocatePhysicalFrame(), TRUE, FALSE, FALSE, FALSE);
}
