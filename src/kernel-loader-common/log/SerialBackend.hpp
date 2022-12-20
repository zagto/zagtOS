#pragma once

#include "setup/HandOverState.hpp"
#ifdef SYSTEM_X86_64
#include <log/PCSerial.hpp>
#endif

class SerialBackend {
private:
    hos_v1::SerialInfo info;
#ifdef SYSTEM_X86_64
    PCSerial pcSerial;
#endif

    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();

public:
    void init(const hos_v1::SerialInfo &serialInfo);
    void writeCharacter(char character);
    void write(char character);
};
