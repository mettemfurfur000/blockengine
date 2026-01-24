#ifndef NETWORK_H
#define NETWORK_H 1

#include "general.h"
#include "update_system.h"
// #include <stdio.h>
// #include "enet/enet.h"

/* ============================================================================
 * PACKET TYPE DEFINITIONS
 * All packets use little-endian encoding via fmemopen and file_system macros
 * ============================================================================ */

typedef enum
{
    /* Control & Connection Packets (Engine) */
    PACKET_TYPE_NONE,

    /* Connection Handshake */
    PACKET_TYPE_CONN_REQUEST,    // Client -> Server: Initial connection request
    PACKET_TYPE_CONN_CHALLENGE,  // Server -> Client: Auth challenge
    PACKET_TYPE_CONN_AUTH,       // Client -> Server: Auth response with credentials
    PACKET_TYPE_CONN_ACCEPTED,   // Server -> Client: Connection accepted, send initial state
    PACKET_TYPE_CONN_REJECTED,   // Server -> Client: Connection rejected with reason
    PACKET_TYPE_CONN_DISCONNECT, // Bidirectional: Clean disconnect

    /* Game State - Block & Layer Updates */
    PACKET_TYPE_LAYER_UPDATE, // Server -> Client: Block updates for a layer
    PACKET_TYPE_LAYER_SYNC,   // Server -> Client: Full layer synchronization (fallback)
    PACKET_TYPE_BLOCK_PLACE,  // Client -> Server: Block placement request
    PACKET_TYPE_BLOCK_REMOVE, // Client -> Server: Block removal request
    PACKET_TYPE_BLOCK_MOVE,   // Client -> Server: Block move request (for pistons, conveyors, etc)

    /* Registry & Asset Management */
    PACKET_TYPE_REGISTRY_REQUEST, // Client -> Server: Request registry file
    PACKET_TYPE_REGISTRY_DATA,    // Server -> Client: Registry file chunk (with zlib compression)
    PACKET_TYPE_REGISTRY_ACK,     // Client -> Server: Acknowledge received registry chunk
    PACKET_TYPE_REGISTRY_HASH,    // Server -> Client: Registry hash for verification

    /* Lua Communication - Custom Game Logic */
    PACKET_TYPE_LUA_RPC_REQUEST,  // Client -> Server: Lua RPC call (script function invocation)
    PACKET_TYPE_LUA_RPC_RESPONSE, // Server -> Client: Lua RPC return value
    PACKET_TYPE_LUA_BROADCAST,    // Server -> Clients: Broadcast Lua event to all/filtered clients
    PACKET_TYPE_LUA_CUSTOM,       // Bidirectional: Custom data payload for Lua scripts (opaque)

    /* Administration & Moderation */
    PACKET_TYPE_KICK,         // Server -> Client: Kick player with reason
    PACKET_TYPE_BAN,          // Server -> Client: Ban player with duration
    PACKET_TYPE_ADMIN_ACTION, // Server: Admin command execution (whitelist, ban, etc)
    PACKET_TYPE_CHAT,         // Bidirectional: In-game chat message

    /* Voice over IP (Future) */
    PACKET_TYPE_VOIP_INIT,  // Client -> Server: Initiate voice session
    PACKET_TYPE_VOIP_FRAME, // Bidirectional: Compressed voice frame
    PACKET_TYPE_VOIP_END,   // Bidirectional: End voice session

    /* System & Heartbeat */
    PACKET_TYPE_PING,      // Bidirectional: Latency check
    PACKET_TYPE_PONG,      // Bidirectional: Latency response
    PACKET_TYPE_HEARTBEAT, // Bidirectional: Keep-alive packet

    PACKET_TYPE_RAW, // Used for raw/debug data, could be used from Lua

    PACKET_TYPE_LAST
} packet_type;

#define MAX_CLIENTS 64

/* ============================================================================
 * SERVER FUNCTIONS
 * ============================================================================ */

// Initialize ENet server on given port with configuration. Returns 0 on success
int net_server_init(u16 port, u32 max_clients);
void net_server_shutdown(void);
void net_test_new_thing(void);

#endif
