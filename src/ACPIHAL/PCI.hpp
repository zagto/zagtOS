#pragma once

#include <zagtos/Messaging.hpp>
extern "C" {
#include <acpi.h>
}

extern "C" uint64_t getPCIConfigAddress(ACPI_PCI_ID *PciId);
void initPCIForACPI();
void initPCIForOS(zagtos::RemotePort &envPort);
