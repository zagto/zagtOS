#include <ACPI.hpp>

static const MADTTable *findMADT(const TableHeader *rsdt) noexcept {
    size_t pointerSize;
    if (memcmp(rsdt->signature, "RSDT", 4) == 0) {
        pointerSize = 4;
    } else if (memcmp(rsdt->signature, "XSDT", 4) == 0) {
        pointerSize = 8;
    } else {
        cout << "Could not detect RSDT-like table type" << endl;
        Panic();
    }

    const uint8_t *pointerPointer = reinterpret_cast<const uint8_t *>(rsdt + 1);
    const TableHeader *pointer;

    while (pointerPointer < reinterpret_cast<const uint8_t *>(rsdt) + rsdt->length) {
        /* assumes litte-endian */
        memcpy(&pointer, pointerPointer, pointerSize);

        if (memcmp(pointer->signature, "APIC", 4) == 0) {
            return static_cast<const MADTTable *>(pointer);
        }

        pointerPointer += pointerSize;
    }

    cout << "Could not find MADT table." << endl;
    Panic();
}

const MADTTable *findMADT(PhysicalAddress root) noexcept{

    const RSDPTable *rsdp = reinterpret_cast<const RSDPTable *>(root.value());
    if (rsdp->revision == 0) {
        cout << "Found ACPI Version 1.0." << endl;
        return findMADT(reinterpret_cast<const TableHeader *>(rsdp->rsdtAddress));
    } else {
        cout << "Found ACPI Version 2.0+." << endl;
        return findMADT(reinterpret_cast<const TableHeader *>(rsdp->xsdtAddress));
    }
}
