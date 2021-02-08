#ifndef DLMALLOCGLUE_HPP
#define DLMALLOCGLUE_HPP

#include <common/utils.hpp>

#ifdef __cplusplus
extern "C" {
#endif
    __attribute__((noreturn)) void BasicDLMallocPanic(const char *location);
    size_t KernelMoreCore(ssize_t increment, void *result);

    #define STR2(x) #x
    #define STR1(x) STR2(x)
    #define STR__LINE__ STR1(__LINE__)
    #define DLMallocPanic() BasicDLMallocPanic(__FILE__ ", Line " STR__LINE__)
#ifdef __cplusplus
}
#endif

#endif // DLMALLOCGLUE_HPP
