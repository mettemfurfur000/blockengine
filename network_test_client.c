#include "src/networking.c"

int main(int argc, char *argv[])
{
	int mysocket;
	struct sockaddr_in servaddr;

	init_client(&mysocket, &servaddr);

	client_print_message(mysocket, &servaddr);

	close(mysocket);

	return EXIT_SUCCESS;
}