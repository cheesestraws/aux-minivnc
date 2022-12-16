#include "session.h"
#include "vnc_types.h"
#include "frame_buffer.h"
#include "frame_buffer_ops.h"
#include "keyboard_mess.h"

#include "../kernel/src/fb.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>

void TODO(char* msg) {
	printf("[unimplemented] %s\n", msg);
}

void session_setupVideoInfo(session* sess);

void sendw(session* sess, char* msg, int len, int flags);
int recvw(session* sess, char* buf, int len, int flags);

void vnc_sendProtocolVersion(session* sess);
void vnc_requestClientProtocolVersion(session* sess);
void vnc_sendSecurityHandshake(session* sess);
void vnc_requestClientInit(session* sess);
void vnc_sendServerInit(session* sess);
void vnc_readAndHandleMessages(session* sess);

void vnc_handleMessageFragments(session* sess, char *src, int len);
int vnc_msg_encodingfragsize(session* sess, int bytesRead);

void vnc_evt_encoding_fragment(session* sess, long bytesRead);
void vnc_evt_client_cut_text(session* sess, VNCClientCutText* evt);
void vnc_evt_key(session* sess, VNCKeyEvent* evt);
void vnc_evt_set_pixel_format(session* sess, VNCSetPixFormat* evt);
void vnc_evt_pointer(session* sess, VNCPointerEvent* evt);
void vnc_evt_update_request(session* sess, VNCFBUpdateReq* evt);

void handle_session(int sock) {
	session sess;
	int ret;
	
	/* Zero our memory */
	memset((char*)(&sess), '\0', sizeof(session));
	
	/* Open framebuffer driver */
	ret = open("/dev/fb0");
	if (ret < 0) {
		printf("couldn't open framebuffer, bailing out...\n");
		sess.fb_fd = -1;
		goto cleanup;
	}
	sess.fb_fd = ret;
	
	/* Set up session information */
	sess.sock = sock;
	sess.last_checkpoint = "handle_session";
	sess.msg_dst = (char*)&sess.messageInProgress;
	
	/* Set up our return jump point for errors */
	ret = setjmp(sess.errjmp);
	if (ret != 0) {
		// An error return returns here.
		printf("got error, cleaning up...\n");
		goto cleanup;	
	}
	
	fb_init(&sess, &sess.fb);
	session_setupVideoInfo(&sess);
	
	/* Now do our protocol handshake, or what we pass off as one. */
	vnc_sendProtocolVersion(&sess);
	vnc_requestClientProtocolVersion(&sess);
	vnc_sendSecurityHandshake(&sess);
	vnc_requestClientInit(&sess);
	vnc_sendServerInit(&sess);
	
	while(1) {
		vnc_readAndHandleMessages(&sess);
	}
	
cleanup:
	fb_free(&sess.fb);
	close(sess.sock);
	close(sess.fb_fd);
}

void session_setupVideoInfo(session* sess) {
	int ret;
	sess->last_checkpoint = "session_setupVideoInfo";

	
	ret = ioctl(sess->fb_fd, FB_METADATA, &(sess->video_info));
	if (ret < 0) {	
	    session_err(sess, "FB_METADATA failed");
	}
}

void session_err(session* sess, char* msg) {
	printf("%s: %s\n", sess->last_checkpoint, msg);
	longjmp(sess->errjmp, 1);
}

/* Utility functions */

void sendw(session* sess, char* msg, int len, int flags) {
	int ret = send(sess->sock, msg, len, flags);
	if (ret < 0) {
		perror(sess->last_checkpoint);
		longjmp(sess->errjmp, 1);
	}
}

int recvw(session* sess, char* buf, int len, int flags) {
	int ret = recv(sess->sock, buf, len, flags);
	if (ret < 0) {
		perror(sess->last_checkpoint);
		longjmp(sess->errjmp, 1);
	}
	if (ret == 0) {
		session_err(sess, "EOF on socket");
	}
	
	return ret;
}


/* VNC protocol gubbins */

void vnc_sendProtocolVersion(session* sess) {
	char* version = "RFB 003.003\n";
	sess->last_checkpoint = "vnc_sendProtocolVersion";
	
	sendw(sess, version, strlen(version), 0);
}

void vnc_requestClientProtocolVersion(session* sess) {
	char vncClientVersion[13];
	sess->last_checkpoint = "vnc_requestClientProtocolVersion";
	
	recvw(sess, vncClientVersion, 12, 0);
}

void vnc_sendSecurityHandshake(session* sess) {
	long vncSecurityHandshake = 1;
	sess->last_checkpoint = "vnc_sendSecurityHandshake";
	sendw(sess, (char*) &vncSecurityHandshake, sizeof(long), 0);
}

void vnc_requestClientInit(session* sess) {
	char vncClientInit;
	sess->last_checkpoint = "vnc_RequestClientInit";
	
	recvw(sess, &vncClientInit, 1, 0);
}

void vnc_sendServerInit(session* sess) {
	VNCServerInit vncServerInit;
	int hostnameLen;
	int seenNull;
	int i;
	
	sess->last_checkpoint = "vnc_sendServerInit";
	
	vncServerInit.fbWidth = sess->video_info.video_scr_x;
	vncServerInit.fbHeight = sess->video_info.video_scr_y;
	vncServerInit.format.bigEndian = 1;
	vncServerInit.format.bitsPerPixel = 8;
	vncServerInit.format.depth = 8;
	vncServerInit.format.trueColour = 0;
	
	// get the hostname
	vncServerInit.nameLength = VNC_SERVER_INIT_NAME_LEN;
	gethostname(vncServerInit.name, VNC_SERVER_INIT_NAME_LEN);
	
	// pad it with spaces
	seenNull = 0;
	for (i = 0; i < VNC_SERVER_INIT_NAME_LEN; i++) {
		if (vncServerInit.name[i] == '\0') {
			seenNull = 1;
		}
		if (seenNull) {
			vncServerInit.name[i] = ' ';
		}
	}
	
	// and send it
	sendw(sess, (char*)&vncServerInit, sizeof(vncServerInit), 0);
}

void vnc_readAndHandleMessages(session* sess) {
	int len;

	sess->last_checkpoint = "vnc_readAndHandleMessages";
	
	len = recvw(sess, sess->recv_buf, SESS_RECV_BUFF_LEN, 0);
	vnc_handleMessageFragments(sess, sess->recv_buf, len);
}

void vnc_handleMessageFragments(session* sess, char *src, int len) {
	unsigned int bytesRead, msgSize;

	sess->last_checkpoint = "vnc_handleMessageFragments";

	/**
	 * Process unfragmented mPointerEvent and mFBUpdateRequest messages,
	 * the common case
	 *
	 * Whole messages will be processed without copying. Upon exit, either
	 * len = 0 and all data has been consumed, or len != 0 and src will
	 * point to messages for further processing.
	 */

	if (sess->msg_dst == (char *)&sess->messageInProgress) {
		// We're not in the middle of a message.
		while(true) {
			if((*src == mPointerEvent) && (len >= sizeof(VNCPointerEvent))) {
				vnc_evt_pointer(sess, (VNCPointerEvent*)src);
				src += sizeof(VNCPointerEvent);
				len -= sizeof(VNCPointerEvent);
			}

			else if((*src == mFBUpdateRequest) && (len >= sizeof(VNCFBUpdateReq))) {
				vnc_evt_update_request(sess, (VNCFBUpdateReq*)src);
				src += sizeof(VNCFBUpdateReq);
				len -= sizeof(VNCFBUpdateReq);
			}

			else
				break; // Exit loop for any other message
		}
	}

	/**
	 * Deal with fragmented messages, a less common case
	 *
	 * While processing fragmented messages, src is a read pointer into the
	 * RDS while sess->msg_ dst is a write pointer into vncClientMessage. Bytes are 
	 * copied from src to msg_dst until a full message is detected, at which point it
	 * is processed and msg_dst is reset to the top of vncClientMessage.
	 *
	 * Certain messages in the VNC protocol are variable size, so a message
	 * may be copied in bits until its total size is known.
	 */

	while(len) {
		sess->last_checkpoint = "vnc_handleMessageFragments";

		// If we've read no bytes yet, get the message type
		if(sess->msg_dst == (char *)&sess->messageInProgress) {
			sess->messageInProgress.message = *src++;
			len--;
			sess->msg_dst++;
		}

		// How many bytes have been read up to this point?
		bytesRead = sess->msg_dst - (char *)&sess->messageInProgress;

		// Figure out the message length
		switch(sess->messageInProgress.message) {
			case mSetPixelFormat:  msgSize = sizeof(VNCSetPixFormat); break;
			case mFBUpdateRequest: msgSize = sizeof(VNCFBUpdateReq); break;
			case mKeyEvent:        msgSize = sizeof(VNCKeyEvent); break;
			case mPointerEvent:    msgSize = sizeof(VNCPointerEvent); break;
			case mClientCutText:   msgSize = sizeof(VNCClientCutText); break;
			case mSetEncodings:    msgSize = vnc_msg_encodingfragsize(sess, bytesRead); break;
			default:
				session_err(sess,"Illegal message");
		}

		// Copy message bytes
		if(bytesRead < msgSize) {
			unsigned long bytesToCopy = msgSize - bytesRead;

			// Copy the message bytes
			if(bytesToCopy > len) bytesToCopy = len;

			memcpy(sess->msg_dst, src, bytesToCopy);
			src += bytesToCopy;
			len -= bytesToCopy;
			sess->msg_dst += bytesToCopy;
		}

		// How many bytes have been read up to this point?
		bytesRead = sess->msg_dst - (char *)&sess->messageInProgress;

		if(bytesRead == msgSize) {
			// Dispatch the message
			switch(sess->messageInProgress.message) {
				// Fixed sized messages
				case mSetPixelFormat:  vnc_evt_set_pixel_format(sess, &sess->messageInProgress.pixFormat); break;
				case mFBUpdateRequest: vnc_evt_update_request(sess, &sess->messageInProgress.fbUpdateReq); break;
				case mKeyEvent:        vnc_evt_key(sess, &sess->messageInProgress.keyEvent); break;
				case mPointerEvent:    vnc_evt_pointer(sess, &sess->messageInProgress.pointerEvent); break;
				case mClientCutText:   vnc_evt_client_cut_text(sess, &sess->messageInProgress.cutText); break;
				// Variable sized messages
				case mSetEncodings:
					vnc_evt_encoding_fragment(sess, bytesRead);
					continue;
			}
			// Prepare to receive next message
			sess->msg_dst = (char*)&sess->messageInProgress;
		}
	}
}

int vnc_msg_encodingfragsize(session* sess, int bytesRead) {
	if (bytesRead >= sizeof(VNCSetEncoding) && 
		sess->messageInProgress.setEncoding.numberOfEncodings) {
		
		return sizeof(VNCSetEncodingOne);
	} else {
		return sizeof(VNCSetEncoding);
	}
}

/* Event handlers */

void vnc_evt_encoding_fragment(session* sess, long bytesRead) {
	sess->last_checkpoint = "vnc_evt_encoding_fragment";
		
	if(bytesRead == sizeof(VNCSetEncodingOne)) {
		//vncEncoding(sess->mesageInProgress.setEncodingOne.encoding);
		
		printf("(encoding) Got encoding %ld\n", sess->messageInProgress.setEncodingOne.encoding);
		
		sess->messageInProgress.setEncodingOne.numberOfEncodings--;
	} 

	if(sess->messageInProgress.setEncoding.numberOfEncodings) {
		// Preare to read the next encoding
		sess->msg_dst = (char *) &sess->messageInProgress.setEncodingOne.encoding;
	} else {
		// Prepare to receive next message
		sess->msg_dst = (char *) &sess->messageInProgress;
	}	
}

void vnc_evt_client_cut_text(session* sess, VNCClientCutText* evt) {
	sess->last_checkpoint = "vnc_evt_client_cut_text";
	
	// Do nothing
}

void vnc_evt_key(session* sess, VNCKeyEvent* evt) {
	keypresses k;
	sess->last_checkpoint = "vnc_evt_key";
	
	printf("keysym: %hx\n", evt->key);
	
	k = sym_to_keypresses(evt->key);
	if (evt->down) {
		printf("key_down:");
		do_key_down(sess, k);
	} else {
		printf("key_up:");
		do_key_up(sess, k);
	}
}

void vnc_evt_set_pixel_format(session* sess, VNCSetPixFormat* evt) {
	const VNCPixelFormat* format = &evt->format;
	sess->last_checkpoint = "vnc_evt_set_pixel_format";
	
	if (format->trueColour || format->depth != 8) {
		// Invalid colour depth
		session_err(sess, "Invalid colour depth");
	}
}

void vnc_evt_pointer(session* sess, VNCPointerEvent* evt) {
	struct fb_mouse newPosition;
	int ret;
	
	sess->last_checkpoint = "vnc_evt_pointer";
	
	newPosition.x = evt->x;
	newPosition.y = evt->y;
	newPosition.button = (evt->btnMask != 0);

	ret = ioctl(sess->fb_fd, FB_MOVE_MOUSE, &newPosition);
	if (ret < 0) {
		session_err(sess, "Could not move pointer");
	}
}

// Send the colourmap of the associated framebuffer out to the VNC connection
void vnc_upd_send_colourmap(session* sess) {
	VNCSetColourMapHeader hdr;
	
	if (sess->fb.clut_changed) {	
		hdr.message = mSetCMapEntries;
		hdr.padding = 0;
		hdr.firstColour = 0;
		hdr.numColours = 256;
	
		sendw(sess, (char*)&hdr, sizeof(hdr), 0);
		sendw(sess, (char*)sess->fb.vnc_clut, 256 * sizeof(VNCColour), 0);
	
		sess->fb.clut_changed = 0;
	}
}

void vnc_upd_send_hdr_raw(session* sess, char* incremental) {
	VNCFBUpdate hdr;
	
	hdr.message = mFBUpdate;
	hdr.padding = 0;
	hdr.numRects = 1;
	hdr.rect.x = 0;
	hdr.rect.y = 0;
	hdr.rect.w = sess->video_info.video_scr_x;
	hdr.rect.h = sess->video_info.video_scr_y;
	hdr.encodingType = mRawEncoding;
	
	// Let's work out whether we really want to do an
	// incremental update: if the CLUT has changed, or if more than a
	// quarter of the screen has changed, it gives a better user experience
	// to throw the whole screen at send(), even though it's technically
	// slower.  This is because the transmit time is longer, but the time
	// it takes the VNC server to draw it is shorter, so it "feels" snappier.
	
	if (sess->fb.clut_changed) {
		*incremental = 0;
	}
	if (FB_RECT_HEIGHT(sess->fb.last_dirty) > (hdr.rect.h / 2) &&
	    FB_RECT_WIDTH(sess->fb.last_dirty) > (hdr.rect.w / 2)) {
	
		*incremental = false;
	}
	
	// if we still want to do an incremental update, fiddle with the
	// update header accordingly.
	if (*incremental) {
		hdr.rect.y = sess->fb.last_dirty.y1;
		hdr.rect.h = FB_RECT_HEIGHT(sess->fb.last_dirty);
		
		hdr.rect.x = sess->fb.last_dirty.x1;
		hdr.rect.w = FB_RECT_WIDTH(sess->fb.last_dirty);
	}
	
	sendw(sess, (char*)&hdr, sizeof(hdr), 0);
}

void vnc_send_body_raw(session* sess, char incremental) {
	int start;
	int size = 0;
	
	if (!incremental) {
		sendw(sess, sess->fb.frame_to_send, sess->fb.frame_size, 0);
	} else {
		/*start = sess->fb.last_dirty.y1 * sess->video_info.video_scr_x;
		size = (sess->fb.last_dirty.y2 + 1 - sess->fb.last_dirty.y1) * sess->video_info.video_scr_x;
		
		sendw(sess, sess->fb.frame_to_send + start, size, 0); */
		
		fb_reset_dirty_cursor(&sess->fb);
		while (fb_more_dirty(&sess->fb)) {
			sendw(sess, fb_dirty_cursor_ptr(&sess->fb), fb_size_at_dirty_cursor(&sess->fb), 0);		
			fb_advance_dirty_cursor(&sess->fb);
		}
		
		
	}
}

void vnc_evt_update_request(session* sess, VNCFBUpdateReq* evt) {
	sess->last_checkpoint = "vnc_evt_update_request";
		
	fb_update(sess, &sess->fb);
	
	vnc_upd_send_colourmap(sess);
	
	vnc_upd_send_hdr_raw(sess, &evt->incremental);
	vnc_send_body_raw(sess, evt->incremental);
}