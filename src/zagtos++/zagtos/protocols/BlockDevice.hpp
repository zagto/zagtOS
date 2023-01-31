#pragma once

#include <zagtos/UUID.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/SharedMemory.hpp>

namespace zagtos {
namespace blockDevice {

static constexpr UUID DEVICE_CLASS(
            0xcf, 0x66, 0xfb, 0xac, 0xef, 0x4f, 0x49, 0x40,
            0x99, 0xbf, 0xcb, 0x81, 0xb0, 0xa7, 0xf9, 0xe2);

struct DeviceInfo {
    uint64_t blockSize;
    uint64_t numBlocks;

    ZBON_ENCODING_FUNCTIONS(blockSize, numBlocks)
};

static constexpr uint32_t ACTION_READ = 1;
static constexpr uint32_t ACTION_WRITE = 2;

static constexpr uint32_t ACTION_RESULT_OK = 1;
static constexpr uint32_t ACTION_RESULT_INVALID = 2;

namespace send {
struct Allocate {
    static constexpr UUID uuid{
            0x9a, 0x2b, 0xa7, 0x20, 0xe2, 0xd2, 0x4e, 0xcd,
            0xbb, 0x0f, 0xad, 0x0d, 0x46, 0x7f, 0x30, 0xe4};

    size_t length;
    Port &responsePort;

    ZBON_ENCODING_FUNCTIONS(length, responsePort);
};

struct AllocateResult {
    static constexpr UUID uuid{
            0xa7, 0x3e, 0x75, 0x80, 0x65, 0x70, 0x45, 0x31,
            0xb3, 0xca, 0x69, 0x92, 0xf8, 0x0a, 0xad, 0x14};

    SharedMemory &sharedMemory;
    Port &port;

    ZBON_ENCODING_FUNCTIONS(sharedMemory, port);
};

struct SubmitAction {
    static constexpr UUID uuid{
            0x9a, 0x87, 0x13, 0x34, 0x77, 0x63, 0x4e, 0x1a,
            0x9e, 0x42, 0x82, 0x0a, 0x9b, 0x66, 0xf7, 0xf1};

    uint32_t action;
    uint32_t cookie;
    size_t startPage;
    size_t startSector;
    size_t numSectors;
    Port &responsePort;

    ZBON_ENCODING_FUNCTIONS(action, cookie, startPage, startSector, numSectors, responsePort);
};

struct ActionComplete {
    static constexpr UUID uuid{
            0x1c, 0xc6, 0x52, 0xd5, 0x53, 0x86, 0x43, 0x99,
            0x80, 0x4e, 0x80, 0xff, 0xf1, 0x16, 0xba, 0x1e};

    uint32_t result;
    uint32_t cookie;

    ZBON_ENCODING_FUNCTIONS(result, cookie);
};
}

namespace receive {
struct Allocate {
    static constexpr UUID uuid{
            0x9a, 0x2b, 0xa7, 0x20, 0xe2, 0xd2, 0x4e, 0xcd,
            0xbb, 0x0f, 0xad, 0x0d, 0x46, 0x7f, 0x30, 0xe4};

    size_t length;
    RemotePort responsePort;

    ZBON_ENCODING_FUNCTIONS(length, responsePort);
};

struct AllocateResult {
    static constexpr UUID uuid{
            0xa7, 0x3e, 0x75, 0x80, 0x65, 0x70, 0x45, 0x31,
            0xb3, 0xca, 0x69, 0x92, 0xf8, 0x0a, 0xad, 0x14};

    SharedMemory sharedMemory;
    RemotePort port;

    ZBON_ENCODING_FUNCTIONS(sharedMemory, port);
};

struct SubmitAction {
    static constexpr UUID uuid{
            0x9a, 0x87, 0x13, 0x34, 0x77, 0x63, 0x4e, 0x1a,
            0x9e, 0x42, 0x82, 0x0a, 0x9b, 0x66, 0xf7, 0xf1};

    uint32_t action;
    uint32_t cookie;
    size_t startPage;
    size_t startSector;
    size_t numSectors;
    RemotePort responsePort;

    ZBON_ENCODING_FUNCTIONS(action, cookie, startPage, startSector, numSectors, responsePort);
};

using send::ActionComplete;
}


}
}
