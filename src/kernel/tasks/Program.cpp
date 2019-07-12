#include <tasks/Program.hpp>

Program::Program(Slice<Vector, u8> _data) : data{_data.copy()}, elf{Slice<Vector, u8>(_data)} {
}
