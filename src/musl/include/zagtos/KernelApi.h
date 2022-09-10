#if defined(ZAGTOS_KERNEL) || defined(ZAGTOS_LOADER)
#else
#include <stdint.h>
#include <stddef.h>
#include <uuid/uuid.h>
#endif

#ifdef __cplusplus
#define _Bool bool
#define _Alignas alignas
extern "C" {
#endif

#ifndef __ZAGTOS_KERNEL_API_H_MESSAGE_DATA
#define __ZAGTOS_KERNEL_API_H_MESSAGE_DATA

struct ZoMessageData {
    uint8_t *data;
    size_t size;
    size_t numHandles;
    _Bool allocatedExternally;
};

#endif

/* ZoMessageData is exposed directly in ZBON.h - allow including it without all the other
 * definitions */
#ifndef KERNEL_API_ONLY_MESSAGE_DATA

#ifndef __ZAGTOS_KERNEL_API_H_MESSAGE_INFO
#define __ZAGTOS_KERNEL_API_H_MESSAGE_INFO

struct ZoMessageInfo {
    /* index into the ports array of a ReceiveMessage call */
    size_t portIndex;
#if defined(ZAGTOS_KERNEL) || defined(ZAGTOS_LOADER)
    _Alignas(16) UUID type;
#else
    _Alignas(16) uuid_t type;
#endif
    struct ZoMessageData data;
};

#endif /* __ZAGTOS_KERNEL_API_H_MESSAGE_INFO */

/* ZoMessageInfo is exposed directly in Messaging.h - allow including it without all the other
 * definitions */
#ifndef KERNEL_API_ONLY_MESSAGE_INFO
#ifndef __ZAGTOS_KERNEL_API_H
#define __ZAGTOS_KERNEL_API_H

struct ZoProcessStartupInfo {
    uint32_t threadHandle;
    uint32_t messageQueueHandle;
    struct ZoMessageInfo runMessage;
};

struct ZoMmapArguments {
    void *startAddress;
    size_t length;
    uint32_t flags;
    size_t offset;
    void *result;
    uint32_t handle;
    uint32_t protection;
};

/* FUTEX_LOCK_PI wants to or a bit with a handle so make sure upmost bit is reserved */
static const uint32_t ZAGTOS_MAX_HANDLES = 0x4000'0000;
static const uint32_t ZAGTOS_INVALID_HANDLE = 0xffffffff;

#endif /* __ZAGTOS_KERNEL_API_H */
#endif /* KERNEL_API_ONLY_MESSAGE_INFO */
#endif /* KERNEL_API_ONLY_MESSAGE_DATA */

#ifdef __cplusplus
} /* extern "C" */
#undef _Bool
#undef _Alignas
#endif
