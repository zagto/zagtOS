#include <Firmware.hpp>
#include <Multiboot.hpp>
#include <iostream>

hos_v1::FirmwareInfo GetFirmwareInfo() {
    size_t result = 0;
    NewACPITag *newTag = MultibootInfo->getTag<NewACPITag>(0);
    if (newTag) {
        result = reinterpret_cast<size_t>(newTag) + sizeof(Tag);
    } else {
        OldACPITag *oldTag = MultibootInfo->getTag<OldACPITag>(0);
        if (oldTag) {
            result = reinterpret_cast<size_t>(oldTag) + sizeof(Tag);
        }
    }

    if (result == 0) {
        cout << "Not ACPI root found in Multiboot info." << endl;
        Panic();
    }

    return {
        .type = hos_v1::FirmwareType::ACPI,
        .rootAddress = result,
        .regionLength = 0
    };
}
