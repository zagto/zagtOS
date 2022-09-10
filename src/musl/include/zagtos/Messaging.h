#ifndef __ZAGTOS_MESSAGING_H
#define __ZAGTOS_MESSAGING_H

#include <zagtos/ZBON.h>
#define KERNEL_API_ONLY_MESSAGE_INFO
#include <zagtos/KernelApi.h>
#undef KERNEL_API_ONLY_MESSAGE_INFO

struct ZoMessageInfo *ZoGetRunMessage(void);

#endif
