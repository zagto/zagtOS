#include <vector>
#include <iostream>
#include <stdexcept>
#include <zagtos/PCI.hpp>
#include <zagtos/HAL.hpp>
#include <PCI.hpp>
extern "C" {
    #include <acpi.h>
}


static std::vector<zagtos::pci::SegmentGroup> segmentGroups;

extern "C"
uint64_t getPCIConfigAddress(ACPI_PCI_ID *PciId) {
    if (segmentGroups.empty()) {
        throw std::logic_error("getPCIConfigAddress called but PCI was not detected\n");
    }

    for (const auto &group: segmentGroups) {
        if (group.segmentNumber == PciId->Segment) {
            if (PciId->Bus < group.busStart || PciId->Bus > group.busEnd) {
                throw std::logic_error("getPCIConfigAddress called for non existing segment group"
                                       + std::to_string(PciId->Segment));
            }

            return group.configBase + ((static_cast<uint64_t>(PciId->Bus - group.busStart)) << 20
                                       | static_cast<uint64_t>(PciId->Device) << 15
                                       | static_cast<uint64_t>(PciId->Function) << 12);
        }
    }

    throw std::logic_error("getPCIConfigAddress called for non existing segment group"
                           + std::to_string(PciId->Segment));
}

void initPCIForACPI() {
    ACPI_TABLE_MCFG *mcfg;
    ACPI_STATUS result = AcpiGetTable(const_cast<ACPI_STRING>(ACPI_SIG_MCFG),
                                      0,
                                      reinterpret_cast<ACPI_TABLE_HEADER **>(&mcfg));
    if (result == AE_NOT_FOUND) {
        std::cout << "MCFG table does not exist. There will be no PCI support." << std::endl;
        return;
    } else if (result) {
        throw std::logic_error("Getting MCFG table failed: " + std::to_string(result));
    }

    size_t mcfgAddr = reinterpret_cast<size_t>(mcfg);
    size_t subtablesAddr = mcfgAddr + 44;
    size_t subtablesEnd = mcfgAddr + mcfg->Header.Length;
    if ((subtablesEnd - subtablesAddr) % sizeof(ACPI_MCFG_ALLOCATION) != 0) {
        throw std::logic_error("MCFG table size does not end at a whole subtable");
    }
    size_t numSubtables = (subtablesEnd - subtablesAddr) / sizeof(ACPI_MCFG_ALLOCATION);

    ACPI_MCFG_ALLOCATION *subtables = reinterpret_cast<ACPI_MCFG_ALLOCATION *>(subtablesAddr);
    segmentGroups.resize(numSubtables);

    for (size_t i = 0; i < numSubtables; i++) {
        segmentGroups[i] = zagtos::pci::SegmentGroup{
            subtables[i].Address,
            subtables[i].PciSegment,
            subtables[i].StartBusNumber,
            subtables[i].EndBusNumber
        };
    }
}

void initPCIForOS(zagtos::RemotePort &envPort) {
    if (segmentGroups.empty()) {
        /* no PCI support */
        return;
    }

    zagtos::sendMessage(envPort, zagtos::MSG_FOUND_CONTROLLER, zagtos::zbon::encode(segmentGroups));
}

