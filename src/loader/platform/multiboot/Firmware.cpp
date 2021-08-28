#include <Firmware.hpp>
#include <Multiboot.hpp>

PhysicalAddress GetFirmwareRoot() {
    NewACPITag *newTag = MultibootInfo->getTag<NewACPITag>(0);
    if (newTag) {
        return reinterpret_cast<size_t>(newTag) + sizeof(Tag);
    }

    OldACPITag *oldTag = MultibootInfo->getTag<OldACPITag>(0);
    if (oldTag) {
        return reinterpret_cast<size_t>(oldTag) + sizeof(Tag);
    }

    cout << "Not ACPI root found in Multiboot info." << endl;
    Panic();
}

hos_v1::FirmwareType GetFirmwareType() {
    return hos_v1::FirmwareType::ACPI;
}
