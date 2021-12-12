#include <ACPI.hpp>

static const MADTTable *findMADTInternal(PhysicalAddress rsdtAddress) noexcept {
#ifdef ZAGTOS_LOADER
    auto rsdt = reinterpret_cast<const TableHeader *>(rsdtAddress.value());
#else
    auto rsdt = rsdtAddress.identityMapped().asPointer<const TableHeader>();
#endif

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
        size_t physicalAddress = 0;
        memcpy(&physicalAddress, pointerPointer, pointerSize);

#ifdef ZAGTOS_LOADER
        pointer = reinterpret_cast<TableHeader *>(physicalAddress);
#else
        pointer = PhysicalAddress(physicalAddress).identityMapped().asPointer<const TableHeader>();
#endif

        if (memcmp(pointer->signature, "APIC", 4) == 0) {
            return static_cast<const MADTTable *>(pointer);
        }

        pointerPointer += pointerSize;
    }

    cout << "Could not find MADT table." << endl;
    Panic();
}

const MADTTable *findMADT(PhysicalAddress root) noexcept{
#ifdef ZAGTOS_LOADER
    size_t rootVirtual = root.value();
#else
    size_t rootVirtual = root.identityMapped().value();
#endif

    const RSDPTable *rsdp = reinterpret_cast<const RSDPTable *>(rootVirtual);
    if (rsdp->revision == 0) {
        return findMADTInternal(rsdp->rsdtAddress);
    } else {
        return findMADTInternal(rsdp->xsdtAddress);
    }
}
