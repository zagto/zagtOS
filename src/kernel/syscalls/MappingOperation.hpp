#pragma once
#include <common/common.hpp>

static const int32_t PROTECTION_READ = 1,
                     PROTECTION_WRITE = 2,
                     PROTECTION_EXECUTE = 4;
static const uint32_t MAP_SHARED = 0x01,
                      MAP_PRIVATE = 0x02,
                      MAP_FIXED = 0x10,
                      MAP_ANONYMOUS = 0x20,
                      MAP_WHOLE = 0x20000000;

bool addressLengthValid(size_t startAddress, size_t length);
