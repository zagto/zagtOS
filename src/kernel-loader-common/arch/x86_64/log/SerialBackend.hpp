#pragma once

class SerialBackend {
private:
    bool isTransmitEmpty();
    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();
    void writeCharacter(char character);

public:
    void init();
    void write(char character);
};
