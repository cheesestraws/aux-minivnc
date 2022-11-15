#ifndef LEAK_H
#define LEAK_H

struct leak_req_t {
	long cmd;
	long param;
};

#define CMD_NONE 0
#define CMD_GET_FD_TYPE 1

struct leak_resp_t {
	long cmd;
	long valid;
	long ret;
};

void init_leak();
long leak_fd_type(long fd);
void close_leak();

#endif