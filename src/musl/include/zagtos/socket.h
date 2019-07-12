#ifndef __ZAGTOS_SOCKET_H
#define __ZAGTOS_SOCKET_H

#include <zagtos/object.h>

typedef struct {
    ZObject object;
    uint16_t domain;
    uint16_t type;
    uint16_t protocol;
} ZSocketMsg;
ZUUID Z_SOCKET_MSG_TYPE = {0x1dc01729fed64941, 0x80f4e9e6c9721902}

typedef struct {
    ZObject object;
    ZUUID socket;
    int error;
} ZSocketResponse;
ZUUID Z_SOCKET_RESPONSE_TYPE = {0x1dc01729fed64941, 0x80f4e9e6c9721902};


ZUUID Z_SHUTDOWN_MSG_TYPE = {0xf13d89d6ebfb4abb, 0xbddae58a518108a3};
ZUUID Z_SHUTDOWN_RESPONSE_TYPE = {0x5b90a5b8a7124f39, 0x98e0122979f5c439};
ZUUID Z_BIND_MSG_TYPE = {0x941b0d47046047de, 0x89a88c0f8f254a19};
ZUUID Z_BIND_RESPONSE_TYPE = {0x618ab0974e1e488f, 0x832846559c9ffbfd};
ZUUID Z_CONNECT_MSG_TYPE = {0x780c9684b0e94d33, 0x98be257c4d243166};
ZUUID Z_CONNECT_RESPONSE_TYPE = {0x5b90a5b8a7124f39, 0x98e0-122979f5c439};

typedef struct {
    ZObject object;
    struct sockaddr address;

}

#endif // SOCKET_H
