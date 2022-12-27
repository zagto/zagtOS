#pragma once

#include "setup/HandOverState.hpp"
#ifdef __x86_64__
#include <log/PCSerial.hpp>
#endif

class SerialBackend {
private:
    hos_v1::SerialInfo info;
#ifdef __x86_64__
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
