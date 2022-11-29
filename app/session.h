#ifndef SESSION_H
#define SESSION_H

#include <setjmp.h>
#include "../kernel/src/fb.h"

#include "vnc_types.h"
#include "frame_buffer.h"

#define SESS_RECV_BUFF_LEN 256

typedef struct session {
	int sock;
	int fb_fd;
	
	// Just a diagnostic name to print out on errors.
	char* last_checkpoint;
	
	jmp_buf errjmp;
	
	struct video video_info;
	
	char recv_buf[SESS_RECV_BUFF_LEN];
	
	VNCClientMessages messageInProgress;
	char* msg_dst; // the current write point wtihin messageInProgress
	
	frame_buffer fb;
	
} session;

void handle_session(int sock);
void session_err(session* sess, char* msg);

#endif