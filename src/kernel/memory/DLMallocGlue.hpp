#pragma once

#include <common/utils.hpp>

#ifdef __cplusplus
#include <lib/Status.hpp>

namespace dlMallocGlue {

class Glue {
private:

public:
    Glue();
    KernelVirtualAddress allocate(size_t length, size_t align);
    Result<KernelVirtualAddress> resize(KernelVirtualAddress address, size_t length);
    void free(KernelVirtualAddress address);
};

extern Glue DLMallocGlue;
}

using dlMallocGlue::DLMallocGlue;

/* C-style functions for dlmalloc to call Page Allocator and Panic */
extern "C" {
#endif
    __attribute__((noreturn)) void BasicDLMallocPanic(const char *location);
    int KernelMUnmap(void *address, size_t length);
    void *KernelMMap(size_t length);

    #define STR2(x) #x
    #define STR1(x) STR2(x)
    #define STR__LINE__ STR1(__LINE__)
    #define DLMallocPanic() BasicDLMallocPanic(__FILE__ ", Line " STR__LINE__)
#ifdef __cplusplus
}
#endif
