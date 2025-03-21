#ifndef NETWORKING
#define NETWORKING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifdef __MINGW32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "engine_types.h"

#define MAXRCVLEN 500
#define PORTNUM 2300

// returns sockaddr, pointing to server machine (used only on client machine)
struct sockaddr_in make_sockaddr(int port_number, uint32_t inaddr);

// acts as recv, just adds null terminator at the end
int recvstr(int socket, char *dest, const size_t maxlen, int flags);

int init_client(int *socket_dest, struct sockaddr_in *servaddr_dest);
int init_server(int *socket_dest, struct sockaddr_in *servaddr_dest);

int server_connections_handler(int socket, struct sockaddr_in *servaddr);
int client_print_message(int socket, struct sockaddr_in *clientaddr);

void finish_networking();

#endif