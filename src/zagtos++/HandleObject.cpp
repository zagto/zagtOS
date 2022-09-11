#include <zagtos/HandleObject.hpp>
#include <zagtos/syscall.h>

namespace zagtos {

HandleObject::HandleObject() {}

zbon::Size HandleObject::ZBONSize() const {
    return {1, 1};
}

void HandleObject::ZBONEncode(zbon::Encoder &encoder) const {
    encoder.encodeHandle(_handle);
}

void HandleObject::ZBONDecode(zbon::Decoder &decoder) {
    assert(_handle == cApi::ZAGTOS_INVALID_HANDLE);

    decoder.decodeHandle(_handle);
}

HandleObject::HandleObject(HandleObject &&other) {
    _handle = other._handle;
    other._handle = cApi::ZAGTOS_INVALID_HANDLE;
}

HandleObject &HandleObject::operator=(HandleObject &&other) {
    this->~HandleObject();
    _handle = other._handle;
    other._handle = cApi::ZAGTOS_INVALID_HANDLE;
    return *this;
}

HandleObject::~HandleObject() {
    if (_handle != cApi::ZAGTOS_INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle);
    }
}

}
