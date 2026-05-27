// Simple cross-platform TCP helpers for small message exchange
#ifndef NET_H
#define NET_H

#include "general.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET net_socket_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef int net_socket_t;
#endif

int net_init(void);
void net_cleanup(void);

// Listen on given port (string, e.g. "12345"). Returns listening socket or -1 on error.
net_socket_t net_listen(const char *port);

// Accept a connection on a listening socket. Returns client socket or -1 on error.
net_socket_t net_accept(net_socket_t listen_sock);

// Connect to host:port. Returns connected socket or -1 on error.
net_socket_t net_connect(const char *host, const char *port);

// Close socket
void net_close(net_socket_t sock);

// Send a length-prefixed message (4-byte network-order length, then payload).
// Returns 0 on success, -1 on error.
int net_send_message(net_socket_t sock, const u8 *data, u32 len);

// Receive a length-prefixed message. Allocates buffer with malloc; caller must free.
// On success, *out_buf points to null-terminated buffer and *out_len contains payload length.
// Returns 0 on success, 1 on connection closed, -1 on error.
int net_recv_message(net_socket_t sock, u8 **out_buf, u32 *out_len);

#endif
