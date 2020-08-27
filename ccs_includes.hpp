#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ccs_client {
	SOCKET client_socket;
	char* client_name;
	time_t start_time;
	time_t end_time;
	ccs_client* next;
};
