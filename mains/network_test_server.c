#include "../src/networking.c"

int main(int argc, char *argv[])
{
	int mysocket;
	struct sockaddr_in servaddr;

	init_server(&mysocket, &servaddr);

	server_connections_handler(mysocket, &servaddr);

	close(mysocket);

	return EXIT_SUCCESS;
}