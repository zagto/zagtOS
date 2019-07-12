#ifndef SERIALBACKEND_HPP
#define SERIALBACKEND_HPP

namespace log {
    class SerialBackend {
    private:
        bool isTransmitEmpty();

    public:
        void init();
        void write(char character);
    };
}

#endif // SERIALBACKEND_HPP
