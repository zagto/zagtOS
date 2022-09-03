#ifndef __ZAGTOS_LEGACY_MESSAGING_H
#define __ZAGTOS_LEGACY_MESSAGING_H


#include <stdint.h>
#include <stddef.h>
#include <zagtos/object.h>
#include <uuid/uuid.h>

#ifdef __cplusplus
extern "C" {
#define _Bool bool
#endif


/* legacy */
typedef struct {
    ZUUID uuid;
} ZMessagePort;

ZMessagePort zagtos_create_port(ZUUID allowed_type, ZUUID allowed_sender_tag);
ZObject *zagtos_wait_message(ZUUID port);
void zagtos_destroy_port(ZUUID port);
_Bool zagtos_send_message(ZUUID target_port, void *obj);

/* most programs should not use zagtos_spawn direcly! */
_Bool zagtos_spawn_process(void *image, size_t image_size, ZObject *run_message);

static const ZUUID SIMPLE_LOG_PROTOCOL = {{0xab523817304b4606, 0x99a6dcfb8527fae8}};
static const ZUUID INPUT_DEVICE_PROTOCOL = {{0x3cecb851c26b41a9, 0x969c0e2db9080f45}};

static const ZUUID PHYSICAL_MEMORY_ACCESS_TAG = {{0x51c1370b203a45e2, 0xa335f43b570a602e}};
static const ZUUID CAN_FIND_CPUS_TAG = {{0x0a1ec630539741dd, 0x8d4f7b54a1bb6475}};


#ifdef __cplusplus
#undef _Bool
}
#endif

#endif
