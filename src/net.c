#include "include/net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static int send_all(net_socket_t sock, const void *buf, size_t len)
{
    const u8 *p = buf;
    size_t remaining = len;
    while (remaining > 0)
    {
#ifdef _WIN32
        int sent = send(sock, (const char *)p, (int)remaining, 0);
#else
        ssize_t sent = send(sock, p, remaining, 0);
#endif
        if (sent <= 0)
            return -1;
        p += sent;
        remaining -= sent;
    }
    return 0;
}

static int recv_all(net_socket_t sock, void *buf, size_t len)
{
    u8 *p = buf;
    size_t remaining = len;
    while (remaining > 0)
    {
#ifdef _WIN32
        int r = recv(sock, (char *)p, (int)remaining, 0);
#else
        ssize_t r = recv(sock, p, remaining, 0);
#endif
        if (r <= 0)
            return (r == 0) ? 1 : -1; // 1 -> closed, -1 -> error
        p += r;
        remaining -= r;
    }
    return 0;
}

int net_init(void)
{
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0 ? 0 : -1;
#else
    (void)0;
    return 0;
#endif
}

void net_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

net_socket_t net_listen(const char *port)
{
    struct addrinfo hints = {0}, *res = NULL, *rp = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int gai = getaddrinfo(NULL, port, &hints, &res);
    if (gai != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return (net_socket_t)-1;
    }

    net_socket_t listen_sock = (net_socket_t)-1;

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        net_socket_t s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == (net_socket_t)-1)
            continue;

        int opt = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

        if (bind(s, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            if (listen(s, 1) == 0)
            {
                listen_sock = s;
                break;
            }
        }

#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
    }

    freeaddrinfo(res);

    return listen_sock;
}

net_socket_t net_accept(net_socket_t listen_sock)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    net_socket_t s = accept(listen_sock, (struct sockaddr *)&addr, &addrlen);
    return s;
}

net_socket_t net_connect(const char *host, const char *port)
{
    struct addrinfo hints = {0}, *res = NULL, *rp = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int gai = getaddrinfo(host, port, &hints, &res);
    if (gai != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return (net_socket_t)-1;
    }

    net_socket_t conn_sock = (net_socket_t)-1;

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        net_socket_t s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == (net_socket_t)-1)
            continue;

        if (connect(s, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            conn_sock = s;
            break;
        }

#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
    }

    freeaddrinfo(res);

    return conn_sock;
}

void net_close(net_socket_t sock)
{
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int net_send_message(net_socket_t sock, const u8 *data, u32 len)
{
    if (!data && len > 0)
        return -1;

    uint32_t netlen = htonl(len);
    if (send_all(sock, &netlen, sizeof(netlen)) != 0)
        return -1;

    if (len > 0 && send_all(sock, data, len) != 0)
        return -1;

    return 0;
}

int net_recv_message(net_socket_t sock, u8 **out_buf, u32 *out_len)
{
    uint32_t netlen = 0;
    int r = recv_all(sock, &netlen, sizeof(netlen));
    if (r != 0)
    {
        return r; // 1 -> closed, -1 -> error
    }

    uint32_t len = ntohl(netlen);

    // reasonable sanity limit (10 MB)
    if (len > 10 * 1024 * 1024)
    {
        fprintf(stderr, "incoming message too large: %u bytes\n", len);
        return -1;
    }

    u8 *buf = malloc(len + 1);
    if (!buf && len > 0)
        return -1;

    if (len > 0)
    {
        r = recv_all(sock, buf, len);
        if (r != 0)
        {
            free(buf);
            return r;
        }
    }

    buf[len] = '\0';
    *out_buf = buf;
    if (out_len)
        *out_len = len;

    return 0;
}

net_socket_t net_create_udp_socket(void)
{
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int gai = getaddrinfo(NULL, "0", &hints, &res);
    if (gai != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return (net_socket_t)-1;
    }

    net_socket_t sock = (net_socket_t)-1;
    for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next)
    {
        net_socket_t s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == (net_socket_t)-1)
            continue;

        if (bind(s, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            sock = s;
            break;
        }

#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
    }

    freeaddrinfo(res);
    return sock;
}

int net_bind_udp(net_socket_t sock, const char *port)
{
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int gai = getaddrinfo(NULL, port, &hints, &res);
    if (gai != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return -1;
    }

    int ret = -1;
    for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next)
    {
        if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            ret = 0;
            break;
        }
    }

    freeaddrinfo(res);
    return ret;
}

int net_get_udp_port(net_socket_t sock)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(sock, (struct sockaddr *)&addr, &addrlen) != 0)
        return -1;

    if (addr.ss_family == AF_INET)
    {
        struct sockaddr_in *in = (struct sockaddr_in *)&addr;
        return ntohs(in->sin_port);
    }
    else if (addr.ss_family == AF_INET6)
    {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&addr;
        return ntohs(in6->sin6_port);
    }

    return -1;
}

int net_send_udp(net_socket_t sock, const char *host, const char *port, const u8 *data, u32 len)
{
    if (!data && len > 0)
        return -1;

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int gai = getaddrinfo(host, port, &hints, &res);
    if (gai != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return -1;
    }

    int ret = -1;
    for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next)
    {
#ifdef _WIN32
        int sent = sendto(sock, (const char *)data, (int)len, 0, rp->ai_addr, (int)rp->ai_addrlen);
#else
        ssize_t sent = sendto(sock, data, len, 0, rp->ai_addr, rp->ai_addrlen);
#endif
        if (sent == (int)len || (sent > 0 && (u32)sent == len))
        {
            ret = 0;
            break;
        }
    }

    freeaddrinfo(res);
    return ret;
}

int net_recv_udp(net_socket_t sock, u8 **out_buf, u32 *out_len, struct sockaddr_storage *addr_out)
{
    if (!out_buf)
        return -1;

    u8 buf[4096];
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

#ifdef _WIN32
    int recvd = recvfrom(sock, (char *)buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);
#else
    ssize_t recvd = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);
#endif

    if (recvd < 0)
        return -1;

    u8 *outbuf = malloc(recvd + 1);
    if (!outbuf)
        return -1;

    memcpy(outbuf, buf, recvd);
    outbuf[recvd] = '\0';

    *out_buf = outbuf;
    if (out_len)
        *out_len = (u32)recvd;
    if (addr_out)
        memcpy(addr_out, &addr, addrlen);

    return 0;
}

