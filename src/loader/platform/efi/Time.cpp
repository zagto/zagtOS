#include <Time.hpp>
#include <EFI.hpp>

using namespace efi;

uint64_t timerFrequency = 0;

void detectTimerFrequency() {
    cout << "Calibrating TSC..." << endl;

    uint64_t start = readTimerValue();
    EFI_STATUS status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->Stall),
                                          1,
                                          250 * 1000); // 250 ms
    uint64_t end = readTimerValue();

    if (EFI_ERROR(status)) {
        cout << "Could not wait time using UEFI: " << statusToString(status) << endl;
        Halt();
    }

    uint64_t result = (end - start) * 4;
    cout << "TSC frequency: " << result << "Hz" << endl;
    timerFrequency = result;
}

void delayMilliseconds(uint64_t ms) {
    assert(timerFrequency != 0);

    uint64_t startValue = readTimerValue();
    uint64_t endValue = startValue + (timerFrequency * ms) / 1000;
    uint64_t now = startValue;
    while (now < endValue) {
        now = readTimerValue();
    }
}
