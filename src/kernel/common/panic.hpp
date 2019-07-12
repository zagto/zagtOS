#ifndef PANIC_HPP
#define PANIC_HPP

__attribute__((noreturn)) void Halt();
__attribute__((noreturn)) void Panic();
void _Assert(bool condition, const char *message = nullptr);

extern "C" __attribute__((noreturn)) void basicHalt();

#define STR2(x) #x
#define STR1(x) STR2(x)
#define STR__LINE__ STR1(__LINE__)
#define Assert(condition) _Assert(condition, __FILE__ ", Line " STR__LINE__)

#endif
