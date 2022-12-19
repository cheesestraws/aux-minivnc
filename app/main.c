#include "session.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

void ignore_sigpipe(void)
{
	struct sigvec sv;
	int ret;
	
	memset(&sv, 0, sizeof(sv));
	sv.sv_handler = SIG_IGN;
	sv.sv_flags = 0;
	
	ret = sigvec(SIGPIPE, &sv, NULL);
	if (ret) {
		perror("aux-minivnc/SIGPIPE");
		printf("sigvec failed! MiniVNC may fall over on client disconnection..\n");
	}
}

void listener(void) {
	int sock;
	int clisock;
	int ret;
	int reuseaddr;
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
	
	/* Enable REUSEADDR so we can come back if we break */
	reuseaddr = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));

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
		printf("accepted VNC connection from %s, starting session...\n", inet_ntoa(cliaddr.sin_addr));
		handle_session(clisock, cliaddr.sin_addr);
	}
}

void credits() {
	printf("minivnc: a VNC server for A/UX.  Under the GPL version 3.\n");
	printf("  (c) Rob Mitchelmore, 2022\n");
	printf("  inspired by and portions (c) Marcio Teixeira, 2022\n");
	printf("  special thanks to SolraBizna for A/UX kernel driver example.\n");
}

int main(int argc, char** argv) {
	if (argc == 2 && (strcmp(argv[1], "--credits") == 0)) {
		credits();
		return;
	}

	ignore_sigpipe();
	nice(20);
	listener();	
}