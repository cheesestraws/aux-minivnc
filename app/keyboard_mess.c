#include "../kernel/src/fb.h"

#include "keyboard_mess.h"

// numbers
static char digits[10] 
	= { 0x1D, 0x12, 0x13, 0x14, 0x15, 0x17, 0x16, 0x1A, 0x1C, 0x19 };

static char letters[26] 
	= { 0x00, 0x0B, 0x08, 0x02, 0x0E, 0x03, 0x05, 0x04, 0x22, 0x26, 
	    0x28, 0x25, 0x2E, 0x2D, 0x1F, 0x23, 0x0C, 0x0F, 0x01, 0x11, 
	    0x20, 0x09, 0x0D, 0x07, 0x10, 0x06};

keypresses sym_to_keypresses(unsigned int keysym) {
	keypresses ks;
	
	ks.count = 0;

	// This is a horrible mess and is rather stupid.
	
	// A digit?
	if ('0' <= keysym && keysym <= '9') {
		ks.count = 1;
		ks.keys[0] = digits[keysym - 0x30];
		return ks;
	}
	
	if ('a' <= keysym && keysym <= 'z') {
		ks.count = 1;
		ks.keys[0] = letters[keysym - 'a'];
		return ks;
	}
}

static void do_key(session* sess, unsigned char code) {
	int ret;
	int chr;
	
	chr = code;
	ret = ioctl(sess->fb_fd, FB_KB_KCHR, &chr);
	if (ret < 0) {
		session_err(sess, "keyboard ioctl failed");
	}
}

void do_key_down(session* sess, keypresses kp) {
	int i;
	
	for (i = 0; i < kp.count; i++) {
		printf("do_key: %d", (int)kp.keys[i]);
	
		do_key(sess, kp.keys[i]);
	}
	printf("\n");
}

void do_key_up(session* sess, keypresses kp) {
	int i;
	unsigned char k;
	
	for (i = kp.count - 1; i >= 0; i--) {
		k = kp.keys[i];
		k = k | 0x80; // key up have top bit set
	
		printf("do_key: %d", (int)k);
		do_key(sess, k);
	}
	printf("\n");
}

