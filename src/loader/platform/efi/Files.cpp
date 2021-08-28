#include <EFI.hpp>
#include <Files.hpp>
#include <log/Logger.hpp>
#include <common/panic.hpp>

using namespace efi;

static EFI_FILE_PROTOCOL *volume{nullptr};


static EFI_FILE_PROTOCOL *findOwnVolume() {
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simpleFileSystem;
    EFI_FILE_PROTOCOL *volume;

    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_GUID simple_file_system_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->HandleProtocol),
                               3,
                               ImageHandle,
                               &loaded_image_protocol,
                               (void **)&loadedImage);
    if (EFI_ERROR(status)) {
        cout << "Could not find Loaded Image Protocol: " << statusToString(status) << endl;
        Halt();
    }

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->HandleProtocol),
                               3,
                               loadedImage->DeviceHandle,
                               &simple_file_system_protocol,
                               (void **)&simpleFileSystem);
    if (EFI_ERROR(status)) {
        cout << "Could not find Simple File System Protocol: " << statusToString(status) << endl;
        Halt();
    }

    status = uefi_call_wrapper(reinterpret_cast<void *>(simpleFileSystem->OpenVolume),
                               2,
                               simpleFileSystem,
                               (void **)&volume);
    if (EFI_ERROR(status)) {
        cout << "Could not open Volume: " << statusToString(status) << endl;
        Halt();
    }
    return volume;
}


void *loadFile(const wchar_t *fileName) {
    if (volume == nullptr) {
        volume = findOwnVolume();
    }

    EFI_FILE_PROTOCOL *file;
    EFI_STATUS status;
    char *fileInfoBuffer[0x100];
    EFI_FILE_INFO *fileInfo = (EFI_FILE_INFO *)fileInfoBuffer;
    UINTN fileInfoSize = 0x100;
    UINTN numPages;
    void *address = nullptr;

    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;

    status = uefi_call_wrapper(reinterpret_cast<void *>(volume->Open),
                               5,
                               volume,
                               &file,
                               fileName,
                               EFI_FILE_MODE_READ,
                               0);
    if (EFI_ERROR(status)) {
        cout << "Could not open file: " << statusToString(status) << endl;
        Halt();
    }

    status = uefi_call_wrapper(reinterpret_cast<void *>(file->GetInfo),
                               4,
                               file,
                               &file_info_guid,
                               &fileInfoSize,
                               fileInfo);
    if (EFI_ERROR(status)) {
        cout << "Could not get file info: " << statusToString(status) << endl;
        Halt();
    }
    numPages = (fileInfo->FileSize + PAGE_SIZE - 1) / PAGE_SIZE;

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->AllocatePages),
                               4,
                               AllocateAnyPages,
                               EfiLoaderData,
                               numPages,
                               (EFI_PHYSICAL_ADDRESS *)&address);
    if (EFI_ERROR(status)) {
        cout << "Could not allocate memory for file: " << statusToString(status) << endl;
        Halt();
    }

    status = uefi_call_wrapper(reinterpret_cast<void *>(file->Read),
                               3,
                               file,
                               &fileInfo->FileSize,
                               address);
    if (EFI_ERROR(status)) {
        cout << "Could not read file: " << statusToString(status) << endl;
        Halt();
    }

    status = uefi_call_wrapper(reinterpret_cast<void *>(file->Close),
                               1,
                               file);
    if (EFI_ERROR(status)) {
        cout << "Could not close file: " << statusToString(status) << endl;
        Halt();
    }
    return address;
}

void *LoadKernelImage() {
    return loadFile(L"ZAGTKERN.ZBN");
}

void *LoadProcessImage() {
    return loadFile(L"SYSENV.ZBN");
}
