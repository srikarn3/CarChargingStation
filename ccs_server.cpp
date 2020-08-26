#include "ccs_includes.hpp"

using namespace std;

int start_server()
{
	printf("Configuring local address...\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *bind_address;
	getaddrinfo(0, "8080", &hints, &bind_address);

	printf("Creating socket...\n");
	SOCKET socket_listen;
	socket_listen = socket(bind_address->ai_family,
			bind_address->ai_socktype, bind_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen)) {
		fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}

	printf("Binding socket to local address...\n");
	if (bind(socket_listen,
		bind_address->ai_addr, bind_address->ai_addrlen)) {
		fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}
	freeaddrinfo(bind_address);

	printf("Listening...\n");
	if (listen(socket_listen, 10) < 0) {
		fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}

	return socket_listen;
}

struct ccs_client {
	SOCKET client_socket;
	char* client_name;
	time_t start_time;
	time_t end_time;
	ccs_client* next;
};

struct ccs_client* client_head;
struct ccs_client* client_tail;

void* request_client_info(void* client)
{
	return NULL;
}

int main()
{
	SOCKET socket_listen;
	if (!ISVALIDSOCKET(socket_listen = start_server())) {
		fprintf(stderr, "start_server() failed.");
		exit(1);
	}

	fd_set master;
	FD_ZERO (&master);
	FD_SET(socket_listen, &master);
	SOCKET max_socket = socket_listen;
	printf("Waiting for connections...\n");

	while (1) {
		fd_set reads = master;
		if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
			fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
			exit(1);
		}

		SOCKET i;
		for(i = 1; i <= max_socket; ++i) {
			if (FD_ISSET(i, &reads)) {
				if (i == socket_listen) {
					struct sockaddr_storage client_address;
					socklen_t client_len = sizeof(client_address);
					SOCKET socket_client = accept(socket_listen, 
							(struct sockaddr*) &client_address, 
							&client_len);
					if (!ISVALIDSOCKET(socket_client)) {
						fprintf(stderr, "accept() failed. (%d)\n", 
								GETSOCKETERRNO());
						return 1;
					}
					FD_SET(socket_client, &master);
					if (socket_client > max_socket)
						max_socket = socket_client;
					char address_buffer[100];
					getnameinfo((struct sockaddr*)&client_address, 
							client_len, 
							address_buffer, sizeof(address_buffer), 0, 0, 
							NI_NUMERICHOST);
					printf("New connection from %s\n", address_buffer);
					const char* init_msg = "Enter your name: ";
					if (send(socket_client, init_msg, strlen(init_msg), 0) < 0) {
						fprintf(stderr, "send() failed. (%d)\n", GETSOCKETERRNO());
						return 1;
					}
				} else {
					char read[1024];
					int bytes_received = recv(i, read, 1024, 0);
					if (bytes_received < 1) {
						struct sockaddr_storage client_address;
						socklen_t client_len = sizeof(client_address);
						char address_buffer[100];
						getsockname(i, (struct sockaddr*) &client_address,
								&client_len);
						getnameinfo((struct sockaddr*)&client_address, 
								client_len, 
								address_buffer, sizeof(address_buffer), 0, 0, 
								NI_NUMERICHOST);
						printf("Closing connection with %s\n", address_buffer);
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						continue;
					}
				}
			} //if FD_ISSET
		} //for i to max_socket
	} //while(1)

	printf("Closing listening socket...\n");
	CLOSESOCKET(socket_listen);

	return 0;
}
