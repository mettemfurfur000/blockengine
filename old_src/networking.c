#include "include/networking.h"
// returns sockaddr, pointing to server machine (used only on client machine)
struct sockaddr_in make_sockaddr(int port_number, uint32_t inaddr)
{
	struct sockaddr_in sa_in;

	memset(&sa_in, 0, sizeof(sa_in));
	sa_in.sin_family = AF_INET;
	sa_in.sin_addr.s_addr = htonl(inaddr);
	sa_in.sin_port = htons(port_number);

	return sa_in;
}

// acts as recv, just adds null terminator at the end
int recvstr(int socket, char *dest, const size_t maxlen, int flags)
{
	int len = recv(socket, dest, maxlen, flags);
	dest[len] = '\0';
	return len;
}

int winsockinit()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		printf("WSAStartup failed with error: %d\n", err);
		return FAIL;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return FAIL;
	}
	else
		printf("The Winsock 2.2 dll was found okay\n");

	return SUCCESS;
}

int init_client(int *socket_dest, struct sockaddr_in *servaddr_dest)
{
	if (!winsockinit())
		return FAIL;

	*socket_dest = socket(AF_INET, SOCK_STREAM, 0);
	*servaddr_dest = make_sockaddr(PORTNUM, INADDR_LOOPBACK);

	if (connect(*socket_dest, (struct sockaddr *)servaddr_dest, sizeof(struct sockaddr_in)))
		return FAIL;
	return SUCCESS;
}

int init_server(int *socket_dest, struct sockaddr_in *servaddr_dest)
{
	if (!winsockinit())
		return FAIL;

	*socket_dest = socket(AF_INET, SOCK_STREAM, 0);
	*servaddr_dest = make_sockaddr(PORTNUM, INADDR_LOOPBACK);

	if (bind(*socket_dest, (struct sockaddr *)servaddr_dest, sizeof(struct sockaddr_in)))
		return FAIL;
	return SUCCESS;
}

int server_connections_handler(int socket, struct sockaddr_in *servaddr)
{
	struct sockaddr_in client;
	socklen_t socksize = sizeof(struct sockaddr_in);
	listen(socket, 1);

	char *msg = "Hello World !\n";

	int consocket = accept(socket, (struct sockaddr *)&client, &socksize);

	int counter = 0;

	while (consocket && counter < 5)
	{
		printf("Incoming connection from %s - sending welcome\n", inet_ntoa(client.sin_addr));
		send(consocket, msg, strlen(msg), 0);
		close(consocket);
		counter++;
		consocket = accept(socket, (struct sockaddr *)&client, &socksize);
	}

	close(socket);
	return EXIT_SUCCESS;
}

int client_print_message(int socket, struct sockaddr_in *clientaddr)
{
	char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */
	int len = recvstr(socket, buffer, MAXRCVLEN, 0);

	printf("Received %s (%d bytes).\n", buffer, len);

	close(socket);

	return SUCCESS;
}

void finish_networking()
{
	WSACleanup();
}