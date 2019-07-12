#ifndef GLOBALREGISTERS_HPP
#define GLOBALREGISTERS_HPP

class Processor;
register Processor *CurrentProcessor asm("r15");

#endif // GLOBALREGISTERS_HPP
