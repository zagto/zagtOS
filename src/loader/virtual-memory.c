#include <efi.h>
#include <framestack.h>
#include <physical-memory.h>
#include <virtual-memory.h>
#include <paging.h>


void InitVirtualMemory() {
    MapAddress((EFI_PHYSICAL_ADDRESS)AllocatePhysicalFrame(), SystemInfo, TRUE, FALSE, FALSE);
}
