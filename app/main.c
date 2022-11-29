#include "session.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>

void listener() {
	int sock;
	int clisock;
	int ret;
	struct sockaddr_in addr;
	struct sockaddr_in cliaddr;
	
	sock = socket(AF_INET, SOCK_STREAM, 6);
	if (sock < 0) {
		printf("create socket: error %d\n", errno);
		return;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(5900);

	ret = bind(sock, &addr, sizeof(addr));
	if (ret < 0) {
		printf("bind error: errno: %d\n", errno);
		return;
	}
	
	ret = listen(sock, 1);
	if (ret < 0) {
		printf("listen error: errno: %d\n", errno);
	}


	while (1) {
		clisock = accept(sock, &cliaddr, sizeof(cliaddr));
		printf("accepted socket, starting session...\n");
		handle_session(clisock);
	}
}

int main(int argc, char** argv) {
	listener();	
}