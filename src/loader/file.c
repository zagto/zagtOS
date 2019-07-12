#include <efi.h>
#include <efilib.h>
#include <util.h>
#include <file.h>


EFI_FILE_PROTOCOL *FindOwnVolume(EFI_HANDLE imageHandle) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simpleFileSystem;
    EFI_FILE_PROTOCOL *volume;

    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_GUID simple_file_system_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;

    status = uefi_call_wrapper(ST->BootServices->HandleProtocol,
                               3,
                               imageHandle,
                               &loaded_image_protocol,
                               (void **)&loadedImage);
    if (EFI_ERROR(status)) {
        LogStatus("Could not find Loaded Image Protocol", status);
        Halt();
    }

    status = uefi_call_wrapper(ST->BootServices->HandleProtocol,
                               3,
                               loadedImage->DeviceHandle,
                               &simple_file_system_protocol,
                               (void **)&simpleFileSystem);
    if (EFI_ERROR(status)) {
        LogStatus("Could not find Simple File System Protocol", status);
        Halt();
    }

    status = uefi_call_wrapper(simpleFileSystem->OpenVolume,
                               2,
                               simpleFileSystem,
                               (void **)&volume);
    if (EFI_ERROR(status)) {
        LogStatus("Could not open Volume", status);
        Halt();
    }
    return volume;
}


void *LoadFile(EFI_FILE_PROTOCOL *volume, const CHAR16 *fileName, UINTN *fileSize) {
    EFI_FILE_PROTOCOL *file;
    EFI_STATUS status;
    char *fileInfoBuffer[0x100];
    EFI_FILE_INFO *fileInfo = (EFI_FILE_INFO *)fileInfoBuffer;
    UINTN fileInfoSize = 0x100;
    UINTN numPages;
    void *address = NULL;

    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;

    Log("Loading ");
    Log16(fileName);
    Log("\n");

    status = uefi_call_wrapper(volume->Open,
                               5,
                               volume,
                               &file,
                               fileName,
                               EFI_FILE_MODE_READ,
                               0);
    if (EFI_ERROR(status)) {
        LogStatus("Could not open file", status);
        Halt();
    }

    status = uefi_call_wrapper(file->GetInfo,
                               4,
                               file,
                               &file_info_guid,
                               &fileInfoSize,
                               fileInfo);
    if (EFI_ERROR(status)) {
        LogStatus("Could not get file info", status);
        Halt();
    }
    numPages = (fileInfo->FileSize + PAGE_SIZE - 1) / PAGE_SIZE;
    Log("numPages: ");
    LogUINTN(numPages);
    Log("\n");

    if (fileSize) {
        *fileSize = fileInfo->FileSize;
    }

    status = uefi_call_wrapper(ST->BootServices->AllocatePages,
                               4,
                               AllocateAnyPages,
                               EfiLoaderData,
                               numPages,
                               (EFI_PHYSICAL_ADDRESS *)&address);
    if (EFI_ERROR(status)) {
        LogStatus("Could not allocate memory for file: %lu", status);
        Halt();
    }

    status = uefi_call_wrapper(file->Read,
                               3,
                               file,
                               &fileInfo->FileSize,
                               address);
    if (EFI_ERROR(status)) {
        LogStatus("Could not read file", status);
        Halt();
    }

    Log("read code from file: ");
    LogUINTN(*(UINTN *)(address + 0x1000));
    Log("\n");



    status = uefi_call_wrapper(file->Close,
                               1,
                               file);
    if (EFI_ERROR(status)) {
        LogStatus("Could not close file", status);
        Halt();
    }
    return address;
}
