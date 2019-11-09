#include <common/common.hpp>

struct SourceLocation {
    const char *file;
    uint32_t line;

    void print() {
        cout << file << ", Line " << line << ": ";
    }
};

extern "C" void __ubsan_handle_pointer_overflow(SourceLocation *location, size_t, size_t) {
    location->print();
    cout << "Pointer Overflow" << endl;
    Panic();
}

extern "C" void __ubsan_handle_type_mismatch_v1(SourceLocation *location, void *, size_t) {
    location->print();
    cout << "Type Mismatch" << endl;
    Panic();
}

extern "C" void __ubsan_handle_shift_out_of_bounds(SourceLocation *location, void *, size_t, size_t) {
    location->print();
    cout << "Shift out of Bounds" << endl;
    Panic();
}

extern "C" void __ubsan_handle_out_of_bounds(SourceLocation *location, void *, size_t) {
    location->print();
    cout << "Access out of Bounds" << endl;
    Panic();
}

extern "C" void __ubsan_handle_divrem_overflow(SourceLocation *location, void *, size_t, size_t) {
    location->print();
    cout << "Division Overflow" << endl;
    Panic();
}

extern "C" void __ubsan_handle_mul_overflow(SourceLocation *location, void *, size_t, size_t) {
    location->print();
    cout << "Multiplication Overflow" << endl;
    Panic();
}

extern "C" void __ubsan_handle_add_overflow(SourceLocation *location, void *, size_t, size_t) {
    location->print();
    cout << "Addition Overflow" << endl;
    Panic();
}

extern "C" void __ubsan_handle_sub_overflow(SourceLocation *location, void *, size_t, size_t) {
    location->print();
    cout << "Subtraction Overflow" << endl;
    Panic();
}

extern "C" void __ubsan_handle_load_invalid_value(SourceLocation *location, void *, size_t) {
    location->print();
    cout << "Load of Invalid Value for Type" << endl;
    Panic();
}

extern "C" void __ubsan_handle_invalid_builtin(SourceLocation *location, void *) {
    location->print();
    cout << "Builtin called in Invalid Way" << endl;
    Panic();
}
