#pragma once

class SerialBackend {
private:
    bool isTransmitEmpty();

public:
    void init();
    void write(char character);
    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();
};
