/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ccs_includes.hpp"

struct ccs_client self_info;

int connect_to_server(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "usage: tcp_client hostname port\n");
		exit(1);
	}

	printf("Configuring remote address...\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *peer_address;
	if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
		fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}

	printf("Remote address is: ");
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
	    address_buffer, sizeof(address_buffer),
	    service_buffer, sizeof(service_buffer),
	    NI_NUMERICHOST);
	printf("%s %s\n", address_buffer, service_buffer);

	printf("Creating socket...\n");
	SOCKET socket_peer;
	socket_peer = socket(peer_address->ai_family,
	    peer_address->ai_socktype, peer_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_peer)) {
		fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}


	printf("Connecting...\n");
	if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
		fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
		exit(1);
	}
	freeaddrinfo(peer_address);

	printf("Connected.\n");
	return socket_peer;
}

void print_self_info()
{
	printf("Name: %s\n", self_info.client_name);
}

void free_self_info()
{
	free(self_info.client_name);
	return;
}

int handle_message(char* message, int bytes_received, SOCKET socket_peer)
{
	int p;
	char send_message[4096];
	if (strncmp(message, "GET\n", strlen("GET\n")) == 0) {
		p = (int) (strlen("GET\n"));
		message += p;
		while (*message != 0) {
			if (strncmp(message, "Client-Info: ", strlen("Client-Info: ")) == 0) {
				p = (int) (strlen("Client-Info: "));
				message += p;
				if (strncmp(message, "client_name", strlen("client_name")) == 0) {
					p = (int) (strlen("client_name"));
					message += p;
					printf("Enter your name: ");
					self_info.client_name = (char*) malloc(sizeof(char) * 1024);
					if (!fgets(self_info.client_name, 1024, stdin)) {
						exit(1);
					}
					self_info.client_name[strlen(self_info.client_name) - 1] = 0;
					sprintf(send_message, "POST\nClient-Info: client_name: %s", self_info.client_name);
					int bytes_sent = send(socket_peer, send_message, strlen(send_message), 0);
					printf("Sent (%d bytes).\n", bytes_sent);
				}
			}
		}
	} else if (strncmp(message, "POST", strlen("POST\r\n")) == 0) {

	}
	printf("Finished handling.\n");
	print_self_info();
	return 0;
}

int main(int argc, char *argv[])
{
	int socket_peer = connect_to_server(argc, argv);
	while(1) {
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(socket_peer, &reads);
		FD_SET(0, &reads);

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
			fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
			return 1;
		}
		if (FD_ISSET(socket_peer, &reads)) {
			char recv_message[4096];
			int bytes_received = recv(socket_peer, recv_message, 4096, 0);
			if (bytes_received < 1) {
				printf("Connection closed by peer.\n");
				break;
			}
			printf("Received (%d bytes):\n", bytes_received);
			handle_message(recv_message, bytes_received, socket_peer);
		}
	} //end while(1)

	printf("Closing socket...\n");
	CLOSESOCKET(socket_peer);
	free_self_info();

	printf("Finished.\n");
	return 0;
}

