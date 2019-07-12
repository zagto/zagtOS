#ifndef PROGRAM_HPP
#define PROGRAM_HPP


#include <lib/Vector.hpp>
#include <tasks/ELF.hpp>


class Program {
private:
    Vector<u8> data;
    ELF elf;

public:
    Program(Slice<Vector, u8> _data);
};

#endif // PROGRAM_HPP
