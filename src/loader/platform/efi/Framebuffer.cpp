#include <EFI.hpp>
#include <Framebuffer.hpp>
#include <memory/ArchRegions.hpp>
#include <iostream>

using namespace efi;

static bool framebufferInitialized{false};
static hos_v1::FramebufferInfo framebufferInfo;

static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *getMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsOutput,
                                                     UINT32 modeNumber) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo;
    UINTN sizeOfInfo;

    status = uefi_call_wrapper(reinterpret_cast<void *>(graphicsOutput->QueryMode),
                               4,
                               graphicsOutput,
                               modeNumber,
                               &sizeOfInfo,
                               &modeInfo);
    if (EFI_ERROR(status)) {
        cout << "Could not query graphics mode information:" << statusToString(status) << endl;
        Halt();
    }

    return modeInfo;
}


static UINT32 isModeUsable(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo) {
    return (modeInfo->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
            || modeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
        && modeInfo->PixelsPerScanLine * modeInfo->VerticalResolution * 4
            <= (FramebufferRegion.length + 1) / 2;
}


static UINT32 rateMode(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo) {
    if (isModeUsable(modeInfo)) {
        return modeInfo->HorizontalResolution * modeInfo->VerticalResolution;
    } else {
        return 0;
    }
}


static void setBestMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsOutput) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo;
    UINT32 currentIndex;
    UINT32 currentScore;
    UINT32 bestIndex = 0;
    UINT32 bestScore = 0;

    // If the current mode is usable for us, assume the UEFI chose the best one
    if (isModeUsable(graphicsOutput->Mode->Info)) {
        return;
    }

    // Otherwise find the one with the greatest resolution
    for (currentIndex = 0; currentIndex < graphicsOutput->Mode->MaxMode; currentIndex++) {
        modeInfo = getMode(graphicsOutput, currentIndex);
        currentScore = rateMode(modeInfo);

        if (currentScore > bestScore) {
            bestScore = currentScore;
            bestIndex = currentIndex;
        }
    }

    if (bestScore == 0) {
        cout << "Could not find a usable Framebuffer mode";
        Halt();
    }

    status = uefi_call_wrapper(reinterpret_cast<void *>(graphicsOutput->SetMode),
                               2,
                               graphicsOutput,
                               bestIndex);
    if (EFI_ERROR(status)) {
        cout << "Could not set best mode:" << statusToString(status) << endl;
        Halt();
    }
}


static void generateFramebufferInfo(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode) {
    framebufferInfo = hos_v1::FramebufferInfo{
        .frontBuffer = reinterpret_cast<uint8_t *>(mode->FrameBufferBase),
        .backBuffer = nullptr, /* inserted by mapFrameBuffer */
        .width = mode->Info->HorizontalResolution,
        .height = mode->Info->VerticalResolution,
        .bytesPerPixel = 4,
        .bytesPerLine = mode->Info->PixelsPerScanLine * 4,
        .format = mode->Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
                ? hos_v1::FramebufferFormat::RGB
                : hos_v1::FramebufferFormat::BGR,
        .scaleFactor = 1,
    };
}


hos_v1::FramebufferInfo &InitFramebuffer() {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsOutput;

    EFI_GUID graphics_output_protocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->LocateProtocol),
                               3,
                               &graphics_output_protocol,
                               nullptr,
                               (void **)&graphicsOutput);
    if (EFI_ERROR(status)) {
        cout << "Could not find Graphics Output Protocol:" << statusToString(status) << endl;
        Halt();
    }

    setBestMode(graphicsOutput);
    generateFramebufferInfo(graphicsOutput->Mode);
    framebufferInitialized = true;
    return framebufferInfo;
}

hos_v1::FramebufferInfo &GetFramebuffer() {
    assert(framebufferInitialized);
    return framebufferInfo;
}
