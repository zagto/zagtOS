#ifndef SERIALBACKEND_HPP
#define SERIALBACKEND_HPP

class SerialBackend {
private:
    bool isTransmitEmpty();

public:
    void init();
    void write(char character);
};

#endif // SERIALBACKEND_HPP
