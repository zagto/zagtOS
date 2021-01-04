#pragma once

#include <cstddef>
#include <zagtos/Register.hpp>

struct PortRegisters {
    REGISTER(CLB)
    REGISTER(CLBU)
    REGISTER(FB)
    REGISTER(FBU)
    REGISTER(IS, BIT(DHRS, 0) BIT(PSS, 1) BIT(DSS, 2) BIT(SDBS, 3) BIT(UFS, 4) BIT(DPS, 5)
             BIT(PCS, 6) BIT(DMPS, 7) BIT(PRCS, 22) BIT(IPMS, 23) BIT(OFS, 24) BIT(INFS, 26)
             BIT(IFS, 27) BIT(HBDS, 28) BIT(HBFS, 29) BIT(TFES, 30) BIT(CPDS, 31))
    REGISTER(IE, BIT(DHRE, 0) BIT(PSE, 1) BIT(DSE, 2) BIT(SDBE, 3) BIT(UFE, 4) BIT(DPE, 5)
             BIT(PCE, 6) BIT(DMPE, 7) BIT(PRCE, 22) BIT(IPME, 23) BIT(OFE, 24) BIT(INFE, 26)
             BIT(IFE, 27) BIT(HBDE, 28) BIT(HBFE, 29) BIT(TFEE, 30) BIT(CPDE, 31))
    REGISTER(CMD, BIT(ST,0) BIT(SUD,1) BIT(POD,2) BIT(CLO,3) BIT(FRE,4) BITS(CCS,8,5) BIT(MPSS,13)
             BIT(FR,14) BIT(CR,15) BIT(CPS,16) BIT(PMA,17) BIT(HPCP,18) BIT(MPSP,19) BIT(CPD,20)
             BIT(ESP,21) BIT(FBSCP,22) BIT(APSTE,23) BIT(ATAPI,24) BIT(DLAE,25) BIT(ALPE,26)
             BIT(ASP,27) BITS(ICC,28,4))
    uint32_t reserved0;
    REGISTER(TFD)
    REGISTER(SIG)
    REGISTER(SSTS, BITS(DET,0,4) BITS(SPD,4,4) BITS(IPM,8,4))
    REGISTER(SCTL, BITS(DET,0,4) BITS(SPD,4,4) BITS(IPM,8,4))
    REGISTER(SERR, BITS(ERR,0,16) BITS(DIAG,16,16))
    uint32_t SATAActive;
    uint32_t commandIssue;
    uint32_t SATANotification;
    uint32_t FISBasedSwitchingControl;
    uint32_t deviceSleep;
    uint8_t reserved1[0x200-0x48];
};

struct ControllerRegisters {
    REGISTER(CAP, BITS(NP,0,5) BIT(SXS,5) BIT(EMS,6) BIT(CCCS,7) BITS(NCS,8,5) BIT(S64A,31))
    REGISTER(GHC, BIT(HR,0) BIT(IE,1) BIT(MSRM,2) BIT(AE,31))
    REGISTER(IS)
    REGISTER(PI)
    REGISTER(VS)
    REGISTER(CCC_CTL)
    REGISTER(CCC_PORTS)
    REGISTER(EM_LOC)
    REGISTER(EM_CTL)
    REGISTER(CAP2, BIT(BOH,0))
    REGISTER(BOHC, BIT(BOS,0) BIT(OOS,1) BIT(BB,4))
};

struct ABAR {
    ControllerRegisters controller;
    uint8_t reserved0[0x100 - sizeof(ControllerRegisters)];
    PortRegisters ports[32];
};

extern size_t MaximumDMAAddress;
