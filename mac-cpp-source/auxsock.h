#ifndef AUXSOCK_H
#define AUXSOCK_H

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct sockaddr {
	unsigned short	sa_family;		/* address family */
	char			sa_data[14];	/* up to 14 bytes of direct address */
};

struct in_addr {
    unsigned long s_addr;
};

struct sockaddr_in {
	short	sin_family;
	short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

#define	AF_UNIX		1		/* local to host (pipes, portals) */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */

#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */

#define	INADDR_ANY		(u_long)0x00000000

/* fcntl(2) requests */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */

// POSIX calls this O_NONBLOCK
#define	O_NDELAY	0x00000004	/* Non-blocking I/O */



long sockerr();

long auxsocket(long domain, long type, long protocol);
long auxbind(long socket, struct sockaddr* address, long address_len);
long auxlisten(long socket, long backlog);
long auxaccept(long socket, struct sockaddr* address, long* address_len);
long auxfcntl(long filedes, long x...);
long auxsend(long sock, char* msg, long len, long flags);
long auxrecv(long sock, char* msg, long len, long flags);
long auxselect(long nfds, unsigned long* readfds, unsigned long* writefds, unsigned long* errfds, struct timeval *timeout);
long auxioctl(unsigned long x...);
long auxopen(char* path, long flags);
long auxclose(long fd);


#endif