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

ccs_client* client_head;
ccs_client* client_tail;

void print_client(ccs_client* temp_client)
{
	printf("%d: %s\n", 
		temp_client->client_socket,
		temp_client->client_name);
}

void print_all_clients()
{
	ccs_client* temp = client_head;
	while (temp != NULL) {
		print_client(temp);
		temp = temp->next;
	}
}

void add_new_client(SOCKET socket_peer)
{
	ccs_client* new_client = (ccs_client*) malloc(sizeof(ccs_client));
	new_client->client_socket = socket_peer;
	new_client->client_name = NULL;
	new_client->next = NULL;

	if (client_head == NULL) {
		client_head = new_client;
		client_tail = new_client;
	} else {
		client_tail->next = new_client;
		client_tail = new_client;
	}
}

void free_client(SOCKET socket_peer)
{
	ccs_client* temp = client_head;
	ccs_client* prev = NULL;
	while (temp->client_socket != socket_peer) {
		prev = temp;
		temp = temp->next;
	}
	if (temp == client_head) {
		client_head = temp->next;
	} else if (temp->next == client_tail) {
		client_tail = prev;
	} else {
		prev->next = temp->next;
	}
	free(temp->client_name);
	free(temp);
}

ccs_client* find_client(SOCKET socket_peer)
{
	ccs_client* temp = client_head;
	while (temp != NULL && temp->client_socket != socket_peer) {
		temp = temp->next;
	}
	return temp;
}

int handle_message(char* message, int bytes_received, SOCKET socket_peer)
{
	int p;
	ccs_client* temp = find_client(socket_peer);
	if (strncmp(message, "POST\r\n", strlen("POST\r\n")) == 0) {
		p = (int) (strlen("POST\r\n"));
		message += p;
		if (strncmp(message, "Client-Info: ", strlen("Client-Info: ")) == 0) {
			p = (int) (strlen("Client-Info: "));
			message += p;
			if (strncmp(message, "client_name: ", strlen("client_name: ")) == 0) {
				p = (int) (strlen("client_name: "));
				message += p;
				temp->client_name = (char*) malloc(sizeof(char) * 1024);
				char* name_end = strstr(message, "\r\n");
				strncpy(temp->client_name, message, name_end - message);
				message = name_end + 2;
			}
		}
	} else if (strncmp(message, "GET\r\n", strlen("GET\r\n")) == 0) {

	} else {
		message++;
	}
	printf("Finished handling.\n");
	print_all_clients();
	return 0;
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
					add_new_client(socket_client);
					const char* init_msg = "GET\r\nClient-Info: client_name\r\n";
					if (send(socket_client, init_msg, strlen(init_msg), 0) < 0) {
						fprintf(stderr, "send() failed. (%d)\n", GETSOCKETERRNO());
						return 1;
					}
				} else {
					char recv_message[1024];
					int bytes_received = recv(i, recv_message, 1024, 0);
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
						free_client(i);
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						print_all_clients();
						continue;
					}
					handle_message(recv_message, bytes_received, i);
				}
			} //if FD_ISSET
		} //for i to max_socket
	} //while(1)

	printf("Closing listening socket...\n");
	CLOSESOCKET(socket_listen);

	return 0;
}
