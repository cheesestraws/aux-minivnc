#include "leak.h"
#include "auxsock.h"
#include "msgbuf.h"

#define printf dprintf

long leakfd = -1;

void init_leak() {
	leakfd = auxopen("/dev/leak0", O_RDWR);
	if (leakfd < 0) {
		printf("opening leak: error: %ld\n", sockerr());
	} else {
		printf("leak FD => %ld\n", leakfd);
	}
}

long leak_fd_type(long fd) {
	long ret;
	struct leak_req_t req = { 0 };
	struct leak_resp_t resp = { 0 };
	
	req.cmd = CMD_GET_FD_TYPE;
	req.param = fd;
	
	ret = auxwrite(leakfd, (char*)&req, sizeof(req));
	if (ret < 0) {
		printf("leak write err: %ld\n", sockerr());
	}
	ret = auxread(leakfd, (char*)&resp, sizeof(resp));
	if (ret < 0) {
		printf("leak %ld read err: %ld\n", leakfd,sockerr());
	}

	
	printf("fd_type of %ld in pid %ld result [%ld, %ld, %ld]\n", fd, auxgetpid(), resp.cmd, resp.valid, resp.ret);
	
	if (resp.cmd == req.cmd && resp.valid) {
		return resp.ret;
	} else {
		return -1;
	}
}

void close_leak() {
	auxclose(leakfd);
	leakfd = -1;
}