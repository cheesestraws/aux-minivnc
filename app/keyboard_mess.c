#include "../kernel/src/fb.h"

#include "keyboard_mess.h"
#include "kchr.h"

/* This file handles keypresses.  Keyboards are a mess, and handling
   them is full of edge cases.
   
   From the client we get X keysyms, which are unsure whether they're
   keycodes or character codes.  Depending on the mode that the A/UX
   keyboard subsystem is in, we either need to send it ASCII characters
   or ADB scancodes.
*/
	    
#define COMMAND 55
#define SHIFT 56
#define LOCK 57
#define OPTION 58
#define CTRL 59

int keysym_is_modifier(unsigned int keysym) {
	if (0xFFE1 <= keysym && keysym <= 0xFFEA) {
		return 1;
	}
	
	if (keysym == 0xFE03) {
		return 1;
	}
	
	if (keysym == 0xff7e) {
		return 1;
	}

	return 0;
}

keypresses vkey_to_keypresses(kchr_vkeypress vk) {
	keypresses ks = { 0 };
	
	if (!vk.valid) {
		return ks;
	}
	
	// check modifiers
	if (vk.modifiers & KCHR_CMD_KEY) {
		KS_APPEND(ks, COMMAND);
	}
	
	if (vk.modifiers & KCHR_SHIFT_KEY) {
		KS_APPEND(ks, SHIFT);
	}
	
	if (vk.modifiers & KCHR_LOCK_KEY) {
		KS_APPEND(ks, LOCK);
	}
	
	if (vk.modifiers & KCHR_OPTION_KEY) {
		KS_APPEND(ks, OPTION);
	}
	
	if (vk.modifiers & KCHR_CTRL_KEY) {
		KS_APPEND(ks, CTRL);
	}
	
	KS_APPEND(ks, vk.keycode);
	
	return ks;
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
		//printf("do_key: %d", (int)kp.keys[i]);
	
		do_key(sess, kp.keys[i]);
	}
	//printf("\n");
}

void do_key_up(session* sess, keypresses kp) {
	int i;
	unsigned char k;
	
	for (i = kp.count - 1; i >= 0; i--) {
		k = kp.keys[i];
		k = k | 0x80; // key up have top bit set
	
		//printf("do_key: %d", (int)k);
		do_key(sess, k);
	}
	//printf("\n");
}

