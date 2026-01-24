#include "include/network.h"
#include "include/endianless.h"
#include "include/general.h"

#include <stdlib.h>
#include <string.h>

#define ENET_IMPLEMENTATION
#include "enet/enet.h"

static ENetHost *server_host = NULL;

int net_server_init(u16 port, u32 max_clients)
{
    if (server_host)
        return 0;

    if (enet_initialize() != 0)
        return -1;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    server_host = enet_host_create(&address, 32, 2, 0, 0);
    if (!server_host)
    {
        enet_deinitialize();
        return -1;
    }

    return 0;
}

void net_server_shutdown(void)
{
    if (server_host)
    {
        enet_host_destroy(server_host);
        server_host = NULL;
    }
    enet_deinitialize();
}

// Packet layout:
// [u8 packet_type][u16 layer_index][u8 true_width][u32 payload_len][payload bytes...]
int net_broadcast_update(u16 layer_index, update_acc *acc, u8 true_width)
{
    if (!server_host || !acc)
        return -1;

    u32 payload_len = acc->block_updates_raw.length;
    u32 header_size = 1 + 2 + 1 + 4;
    u32 total = header_size + payload_len;

    u8 *buf = malloc(total);
    if (!buf)
        return -1;

    u8 *p = buf;
    p[0] = (u8)PACKET_TYPE_LAYER_UPDATE;
    p += 1;

    // write layer_index (2 bytes)
    memcpy(p, &layer_index, 2);
    make_endianless(p, 2);
    p += 2;

    *p = true_width;
    p += 1;

    // payload length (4 bytes)
    memcpy(p, &payload_len, 4);
    make_endianless(p, 4);
    p += 4;

    // payload
    if (payload_len)
        memcpy(p, acc->block_updates_raw.data, payload_len);

    ENetPacket *packet = enet_packet_create(buf, total, ENET_PACKET_FLAG_RELIABLE);
    free(buf);

    if (!packet)
        return -1;

    enet_host_broadcast(server_host, 0, packet);
    enet_host_flush(server_host);

    return 0;
}
