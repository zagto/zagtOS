/* for nanosleep */
#define _GNU_SOURCE 1
#include <iostream>
#include <memory>
#include <ctime>
#include <sys/mman.h>
#include <zagtos/Controller.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/PCI.hpp>

using namespace zagtos;
using namespace zagtos::pci;

static const uint32_t
    CAP_NP_MASK = 0b11111,
    CAP_NP_SHIFT = 0,
    CAP_SXS = 1<<5,
    CAP_EMS = 1<<6,
    CAP_CCCS = 1<<7,
    CAP_NCS_MASK = 0b11111<<8,
    CAP_NCS_SHIFT = 8,
    CAP_S64A = 1<<31,

    CAP2_BOH = 1<<0,
    CAP2_NVMP = 1<<1,
    CAP2_APST = 1<<2,
    CAP2_SDS = 1<<3,
    CAP2_SADM = 1<<4,
    CAP2_DESO = 1<<5,

    BOHC_BOS = 1<<0,
    BOHC_OOS = 1<<1,
    BOHC_SOOE = 1<<2,
    BOHC_OOC = 1<<3,
    BOHC_BB = 1<<4,

    GHC_HBA_RESET = 1<<0,
    GHC_INTERRUPT_ENABLE = 1<<1,
    GHC_MSI_REVERT_TO_SINGLE_MESSAGE = 1<<2,
    GHC_AHCI_ENABLE = 1<<31,

    Px_INTR_DHRE = 1<<0,
    Px_INTR_DSS = 1<<2,
    Px_INTR_SDBS = 1<<3,
    Px_INTR_UFS = 1<<4,
    Px_INTR_DPS = 1<<5,
    Px_INTR_PCS = 1<<6,

    PxCMD_ST = 1<<0,
    PxCMD_SUD = 1<<1,
    PxCMD_POD = 1<<2,
    PxCMD_CLO = 1<<3,
    PxCMD_FRE = 1<<4,
    PxCMD_CCS_SHIFT = 8,
    PxCMD_MPSS = 1<<13,
    PxCMD_FR = 1<<14,
    PxCMD_CR = 1<<15,
    PxCMD_CPS = 1<<16,
    PxCMD_PMA = 1<<17,
    PxCMD_HPCP = 1<<18,
    PxCMD_MPSP = 1<<19,
    PxCMD_CPD = 1<<20,
    PxCMD_ESP = 1<<21,
    PxCMD_FBSCP = 1<<22,
    PxCMD_APSTE = 1<<23,
    PxCMD_ATAPI = 1<<24,
    PxCMD_DLAE = 1<<25,
    PxCMD_ALPE = 1<<26,
    PxCMD_ASP = 1<<27,
    PxCMD_ICC_SHIFT = 28,

    PxSSTS_DET_MASK = 0b1111,
    PxSSTS_DET_SHIFT = 0,
    PxSSTS_DET_PRESENT = 3,
    PxSSTS_DET_NONE = 0,
    PxSSTS_IPM_MASK = 0b1111<<8,
    PxSSTS_IPM_SHIFT = 8,
    PxSSTS_IPM_ACTIVE = 1,
    PxSSTS_IPM_PARTIAL = 2,
    PxSSTS_IPM_SLUMBER = 6,
    PxSSTS_IPM_DEVSLEEP = 8,

    PxSERR_ERR_I = 1<<0,
    PxSERR_ERR_M = 1<<1,
    PxSERR_ERR_T = 1<<8,
    PxSERR_ERR_C = 1<<9,
    PxSERR_ERR_P = 1<<10,
    PxSERR_ERR_E = 1<<11,
    PxSERR_ERR = PxSERR_ERR_I|PxSERR_ERR_M|PxSERR_ERR_T|PxSERR_ERR_C|PxSERR_ERR_P|PxSERR_ERR_E;
static bool Support64Bit;
static size_t NumCommandSlots;


void setBit(volatile uint32_t &_register, uint32_t bit) {
    _register = _register | bit;
}

void clearBit(volatile uint32_t &_register, uint32_t bit) {
    _register = _register & ~bit;
}

struct PortRegisters {
    uint32_t commandListBase;
    uint32_t commandListBaseUpper;
    uint32_t FISBase;
    uint32_t FISBaseUpper;
    uint32_t interruptStatus;
    uint32_t interruptEnable;
    uint32_t commandStatus;
    uint32_t reserved0;
    uint32_t taskFileData;
    uint32_t signature;
    uint32_t SATAStatus;
    uint32_t SATAControl;
    uint32_t SATAError;
    uint32_t SATAActive;
    uint32_t commandIssue;
    uint32_t SATANotification;
    uint32_t FISBasedSwitchingControl;
    uint32_t deviceSleep;
    uint8_t reserved1[0x200-0x48];

    bool isIdle() volatile {
        return !(commandStatus & (PxCMD_ST|PxCMD_CR|PxCMD_FRE|PxCMD_FR));
    }
    void ensureNotRunning() volatile {
        if (!isIdle()) {
            std::cout << "Port is not idle. Attemting to change." << std::endl;
            clearBit(commandStatus, PxCMD_ST);
            while (commandStatus & PxCMD_CR) {}
            if (commandStatus & PxCMD_FRE) {
                clearBit(commandStatus, PxCMD_FRE);
                while (commandStatus & PxCMD_FR) {}
            }
            assert(isIdle());
        }
    }
    uint32_t DET() volatile {
        return (SATAStatus & PxSSTS_DET_MASK) >> PxSSTS_DET_SHIFT;
    }
    uint32_t IPM() volatile {
        return (SATAStatus & PxSSTS_IPM_MASK) >> PxSSTS_IPM_SHIFT;
    }
};

static volatile struct ABARStruct {
    uint32_t HostCapabilities;
    uint32_t globalHostControl;
    uint32_t interruptStatus;
    uint32_t portsImplemented;
    uint32_t version;
    uint32_t CCCCtl;
    uint32_t CCCPorts;
    uint32_t enclosureManagementLocation;
    uint32_t enclosureManagementControl;
    uint32_t hostCapabilitiesExtended;
    uint32_t BIOSHandoff;
    uint8_t reserved0[0x100 - 0x2c];
    PortRegisters ports[32];
} *ABAR;

struct FISStruct {

};

struct CommandList {

};

struct ImplementedPort {
    volatile PortRegisters *registers;
    FISStruct *FIS;
    CommandList *commandList;

    void allocateMemory() {
        size_t deviceMax;
        std::vector<size_t> deviceAddressFIS;
        std::vector<size_t> deviceAddressCommandList;
        if (Support64Bit) {
            deviceMax = std::numeric_limits<size_t>::max();
        } else {
            deviceMax = std::numeric_limits<uint32_t>::max();
        }
        auto sharedMemoryFIS = SharedMemory::DMA(deviceMax, sizeof(FISStruct), deviceAddressFIS);
        auto sharedMemoryCommandList = SharedMemory::DMA(deviceMax, sizeof(CommandList), deviceAddressCommandList);

        /* These structures need to always fit in one page */
        assert(deviceAddressFIS.size() == 1);
        assert(deviceAddressCommandList.size() == 1);

        FIS = sharedMemoryFIS.map<FISStruct>(PROT_READ|PROT_WRITE);
        commandList = sharedMemoryCommandList.map<CommandList>(PROT_READ|PROT_WRITE);

        registers->FISBase = static_cast<uint32_t>(deviceAddressFIS[0]);
        registers->commandListBase = static_cast<uint32_t>(deviceAddressCommandList[0]);
        if (Support64Bit) {
            registers->FISBaseUpper = deviceAddressFIS[0] >> 32;
            registers->FISBaseUpper = deviceAddressCommandList[0] >> 32;
        }
    }
};
static std::vector<ImplementedPort> ImplementedPorts;

struct Device {
    ImplementedPort &port;
};
//static std::vector<Device> Devices;

void determinePortsImplemented() {
    uint32_t reg = ABAR->portsImplemented;
    for (size_t port = 0; port < 32; port++) {
        if (reg & (1<<port)) {
            ImplementedPorts.push_back({&ABAR->ports[port], nullptr, nullptr});
        }
    }
    std::cout << "This controller implements " << ImplementedPorts.size() << " ports." << std::endl;
}

void ensureControllerNotRunning() {
    for (ImplementedPort &port: ImplementedPorts) {
        port.registers->ensureNotRunning();
    }
    std::cout << "All ports are now in idle state." << std::endl;
}

void allocatePortMemory() {
    for (ImplementedPort &port: ImplementedPorts) {
        port.allocateMemory();
    }
    __sync_synchronize();
    for (ImplementedPort &port: ImplementedPorts) {
        setBit(port.registers->commandStatus, PxCMD_FRE);
    }
}

void setupErrorRegister() {
    __sync_synchronize();
    for (ImplementedPort &port: ImplementedPorts) {
        setBit(port.registers->SATAError, PxSERR_ERR);
    }
}

void resetController() {
    setBit(ABAR->globalHostControl, GHC_HBA_RESET);
    setBit(ABAR->globalHostControl, GHC_AHCI_ENABLE);
}

void BIOSHandoff() {
    if (!(ABAR->hostCapabilitiesExtended & CAP2_BOH)) {
        std::cout << "No BIOS Handoff supported" << std::endl;
        return;
    }
    setBit(ABAR->BIOSHandoff, BOHC_OOS);
    while (ABAR->BIOSHandoff & BOHC_BOS) {}

    timespec ts{0, 25000};
    nanosleep(&ts, nullptr);

    while (ABAR->BIOSHandoff & BOHC_BB) {}
    std::cout << "BIOS Handoff finished" << std::endl;
}

void enableInterrupts() {
    __sync_synchronize();
    for (ImplementedPort &port: ImplementedPorts) {
        port.registers->interruptStatus = 0;
    }
    __sync_synchronize();
    ABAR->interruptStatus = 0;
    for (ImplementedPort &port: ImplementedPorts) {
        port.registers->interruptEnable =
                Px_INTR_DHRE|Px_INTR_DSS|Px_INTR_SDBS|Px_INTR_UFS|Px_INTR_DPS|Px_INTR_PCS;

    }
    setBit(ABAR->globalHostControl, GHC_INTERRUPT_ENABLE);
}

void detectDevices() {
    for (ImplementedPort &port: ImplementedPorts) {
        uint32_t det = port.registers->DET();
        if (det == PxSSTS_DET_PRESENT) {
            if (port.registers->IPM() == PxSSTS_IPM_ACTIVE) {
                std::cout << "device signature: " << port.registers->signature << std::endl;
            } else {
                std::cout << "device is not in active IPM state" << std::endl;
            }
        } else if (det != PxSSTS_DET_NONE) {
            std::cout << "device in unsupported detection state: " << det << std::endl;
        }
    }
}

int main() {
    pci::Device dev = decodeRunMessage<pci::Device>(MSG_START_PCI_DRIVER);
    std::cout << "Hello from AHCI" << std::endl;

    if (!dev.BAR[5]) {
        throw std::runtime_error("PCI device claims to be AHCI device but does not implement BAR 5");
    }
    pci::BaseRegister &ABARDesc = *dev.BAR[5];
    ABAR = ABARDesc.sharedMemory.map<ABARStruct>(PROT_READ|PROT_WRITE);
    assert(ABAR != nullptr);

    std::cout << "Mapped ABAR" << std::endl;

    setBit(ABAR->globalHostControl, GHC_AHCI_ENABLE);
    BIOSHandoff();
    resetController();
    determinePortsImplemented();
    ensureControllerNotRunning();
    NumCommandSlots = ((ABAR->HostCapabilities & CAP_NCS_MASK) >> CAP_NCS_SHIFT) + 1;
    std::cout << "This controller supports " << NumCommandSlots << " command slots" << std::endl;
    Support64Bit = (ABAR->HostCapabilities & CAP_S64A);
    std::cout << (Support64Bit ? "Support" : "No support") << " for 64-bit addresses" << std::endl;
    allocatePortMemory();
    setupErrorRegister();
    enableInterrupts();
    detectDevices();

    __sync_synchronize();
    std::cout << "Controller initialization OK" << std::endl;
}
