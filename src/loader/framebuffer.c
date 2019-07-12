#include <efi.h>
#include <efilib.h>
#include <util.h>
#include <framebuffer.h>


static struct FramebufferInfo framebufferInfo;


static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *getMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsOutput,
                                                     UINT32 modeNumber) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo;
    UINTN sizeOfInfo;

    status = uefi_call_wrapper(graphicsOutput->QueryMode,
                               4,
                               graphicsOutput,
                               modeNumber,
                               &sizeOfInfo,
                               &modeInfo);
    if (EFI_ERROR(status)) {
        LogStatus("Could not query graphics mode information", status);
        Halt();
    }

    return modeInfo;
}


static UINT32 isModeUsable(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo) {
    return modeInfo->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
        || modeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor;
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
        Log("Could not find a usable Framebuffer mode");
        Halt();
    }

    status = uefi_call_wrapper(graphicsOutput->SetMode,
                               2,
                               graphicsOutput,
                               bestIndex);
    if (EFI_ERROR(status)) {
        LogStatus("Could not set best mode", status);
        Halt();
    }
}


void generateFramebufferInfo(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode) {
    framebufferInfo = (struct FramebufferInfo) {
        .baseAddress = (UINTN)mode->FrameBufferBase,
        .width = mode->Info->HorizontalResolution,
        .height = mode->Info->VerticalResolution,
        .bytesPerPixel = 4,
        .bytesPerLine = mode->Info->PixelsPerScanLine * 4,
        .format = mode->Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor
                ? FRAMEBUFFER_FORMAT_RGB
                : FRAMEBUFFER_FORMAT_BGR
    };
}


struct FramebufferInfo *InitFramebuffer(void) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsOutput;

    EFI_GUID graphics_output_protocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    status = uefi_call_wrapper(ST->BootServices->LocateProtocol,
                               3,
                               &graphics_output_protocol,
                               NULL,
                               (void **)&graphicsOutput);
    if (EFI_ERROR(status)) {
            LogStatus("Could not find Graphics Output Protocol", status);
            Halt();
        }

    setBestMode(graphicsOutput);
    generateFramebufferInfo(graphicsOutput->Mode);
    return &framebufferInfo;
}
