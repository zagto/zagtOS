#ifndef PROGRAM_HPP
#define PROGRAM_HPP


#include <vector>
#include <processes/ELF.hpp>


class Program {
private:
    vector<uint8_t> data;
    ELF elf;

public:
    Program(Slice<vector, uint8_t> _data);
};

#endif // PROGRAM_HPP
