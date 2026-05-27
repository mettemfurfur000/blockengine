#include "include/arena.h"
#include "include/net.h"
#include "include/tkv.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <process.h>

#define fileno _fileno
#else
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif


static void escape_str(const char *in, char *out, size_t out_sz)
{
	size_t oi = 0;
	for (size_t i = 0; in[i] != '\0' && oi + 2 < out_sz; ++i)
	{
		char c = in[i];
		if (c == '\\' || c == '"')
		{
			out[oi++] = '\\';
			out[oi++] = c;
		}
		else if (c == '\n')
		{
			if (oi + 2 < out_sz)
			{
				out[oi++] = '\\';
				out[oi++] = 'n';
			}
		}
		else
		{
			out[oi++] = c;
		}
	}
	out[oi] = '\0';
}


#if defined(_WIN32)
// Thread helper for reading stdin on Windows (select() doesn't accept console fds)
typedef struct {
	net_socket_t sock;
	const char *name;
} stdin_thread_args;

static unsigned __stdcall stdin_thread_func(void *param)
{
	stdin_thread_args *args = (stdin_thread_args *)param;
	char line[4096];

	while (fgets(line, sizeof(line), stdin))
	{
		size_t l = strlen(line);
		if (l > 0 && line[l - 1] == '\n')
			line[l - 1] = '\0';

		char esc[8192];
		escape_str(line, esc, sizeof(esc));

		char msgbuf[16384];
		snprintf(msgbuf, sizeof(msgbuf), "{ str name = \"%s\"; str msg = \"%s\"; }", args->name, esc);

		if (net_send_message(args->sock, (const u8 *)msgbuf, (u32)strlen(msgbuf)) != 0)
			break;
	}

	free(args);
	return 0;
}
#endif

static void format_print_win_error(i32 error_code)
{
    LPVOID msg_buf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msg_buf,
        0,
        NULL);

    fprintf(stderr, "Error %d: %s\n", error_code, (char *)msg_buf);
    LocalFree(msg_buf);
}

static void print_usage(void)
{
	printf("Usage:\n");
	printf("  p2p_chat -l PORT [-n NAME]   # listen on PORT\n");
	printf("  p2p_chat HOST PORT [-n NAME] # connect to HOST:PORT\n");
}

int main(int argc, char **argv)
{
	const char *name = "anon";
	int listen_mode = 0;
	const char *host = NULL;
	const char *port = NULL;

	if (argc < 2)
	{
		print_usage();
		return 1;
	}

	// Simple arg parsing
	int i = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], "-n") == 0)
		{
			if (i + 1 < argc)
			{
				name = argv[i + 1];
				i += 2;
			}
			else
			{
				print_usage();
				return 1;
			}
		}
		else if (strcmp(argv[i], "-l") == 0)
		{
			if (i + 1 < argc)
			{
				listen_mode = 1;
				port = argv[i + 1];
				i += 2;
			}
			else
			{
				print_usage();
				return 1;
			}
		}
		else
		{
			if (!host)
			{
				host = argv[i];
			}
			else if (!port)
			{
				port = argv[i];
			}
			i++;
		}
	}

	if (!port)
	{
		print_usage();
		return 1;
	}

	if (net_init() != 0)
	{
		fprintf(stderr, "net_init failed\n");
		return 1;
	}

	net_socket_t sock = (net_socket_t)-1;

	if (listen_mode)
	{
		net_socket_t listen_sock = net_listen(port);
		if (listen_sock == (net_socket_t)-1)
		{
			fprintf(stderr, "listen failed\n");
			net_cleanup();
			return 1;
		}

		printf("Listening on port %s...\n", port);
		sock = net_accept(listen_sock);
		if (sock == (net_socket_t)-1)
		{
			fprintf(stderr, "accept failed\n");
			net_close(listen_sock);
			net_cleanup();
			return 1;
		}

		net_close(listen_sock);
		printf("Client connected.\n");
	}
	else
	{
		if (!host || !port)
		{
			print_usage();
			net_cleanup();
			return 1;
		}

		printf("Connecting to %s:%s...\n", host, port);
		sock = net_connect(host, port);
		if (sock == (net_socket_t)-1)
		{
			fprintf(stderr, "connect failed\n");
			net_cleanup();
			return 1;
		}

		printf("Connected.\n");
	}

		// On Windows spawn a dedicated stdin reader thread because select() can't watch console fds
	#if defined(_WIN32)
		{
			stdin_thread_args *args = (stdin_thread_args *)malloc(sizeof(stdin_thread_args));
			if (args)
			{
				args->sock = sock;
				args->name = name;
				uintptr_t thr = _beginthreadex(NULL, 0, stdin_thread_func, args, 0, NULL);
				if (thr == 0)
				{
					free(args);
					fprintf(stderr, "_beginthreadex failed\n");
				}
				else
				{
					CloseHandle((HANDLE)thr);
				}
			}
		}
	#endif

		// Main loop: select on stdin and socket
	while (1)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

	#if !defined(_WIN32)
		FD_SET(fileno(stdin), &readfds);
		int maxfd = (int)sock > fileno(stdin) ? (int)sock : fileno(stdin);
	#else
		int maxfd = (int)sock;
	#endif

		int sel = select(maxfd + 1, &readfds, NULL, NULL, NULL);
		if (sel < 0)
		{
	#if defined(_WIN32)
		    i32 error = WSAGetLastError();
		    format_print_win_error(error);
	#else
		    perror("select");
	#endif
		    break;
		}

		if (FD_ISSET(sock, &readfds))
		{
			u8 *buf = NULL;
			u32 len = 0;
			int rr = net_recv_message(sock, &buf, &len);
			if (rr == 1)
			{
				printf("Connection closed by peer.\n");
				break;
			}
			if (rr != 0)
			{
				fprintf(stderr, "recv error\n");
				break;
			}

			// Try to parse as TKV
			const char *src = (const char *)buf;
			arena *scratch = arena_create(4096);
			arena *tkv_arena = arena_create(8192);
			tkv_object obj = tkv_parse_object(&src, scratch, tkv_arena);
			if (!obj)
			{
				printf("[raw] %s\n", buf);
			}
			else
			{
				tkv_value v = tkv_get_value(obj, "msg");
				if (v.meta.whole != UINT_MAX)
				{
					char *msg = tkv_value_to_str(v);
					tkv_value vs = tkv_get_value(obj, "name");
					char *nm = (vs.meta.whole != UINT_MAX) ? tkv_value_to_str(vs) : "peer";
					printf("%s: %s\n", nm, msg);
				}
				else
				{
					printf("[tkv] could not find 'msg' key\n");
				}
			}

			arena_destroy(tkv_arena);
			arena_destroy(scratch);
			free(buf);
		}

#if !defined(_WIN32)
		if (FD_ISSET(fileno(stdin), &readfds))
		{
			char line[4096];
			if (!fgets(line, sizeof(line), stdin))
			{
				// EOF
				printf("EOF on stdin, exiting.\n");
				break;
			}

			// strip newline
			size_t l = strlen(line);
			if (l > 0 && line[l - 1] == '\n')
				line[l - 1] = '\0';

			char esc[8192];
			escape_str(line, esc, sizeof(esc));

			char msgbuf[16384];
			snprintf(msgbuf, sizeof(msgbuf), "{ str name = \"%s\"; str msg = \"%s\"; }", name, esc);

			if (net_send_message(sock, (const u8 *)msgbuf, (u32)strlen(msgbuf)) != 0)
			{
				fprintf(stderr, "send failed\n");
				break;
			}
		}
#endif
	}

	net_close(sock);
	net_cleanup();

	return 0;
}
