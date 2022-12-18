#include "../kernel/src/fb.h"

#include "keyboard_mess.h"
#include "kchr.h"
#include "keycodes.h"
#include "keysymdef.h"

/* This file handles keypresses.  Keyboards are a mess, and handling
   them is full of edge cases.
   
   From the client we get X keysyms, which are unsure whether they're
   keycodes or character codes.  Depending on the mode that the A/UX
   keyboard subsystem is in, we either need to send it ASCII characters
   or ADB scancodes.
*/
	    
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

keypresses keysym_special_to_keypresses(unsigned int keysym) {
	keypresses kp = { 0 };
	
    switch(keysym) {
        case XK_Shift_L:
        case XK_Shift_R:
            KS_APPEND(kp, SHIFT);
            break;

        case XK_Control_L:
        case XK_Control_R:
			KS_APPEND(kp, CTRL);
            break;

        case XK_Caps_Lock:
			KS_APPEND(kp, LOCK);
            break;

        case XK_Meta_L:
        case XK_Meta_R:
        case XK_Super_L:
        case XK_Super_R:
        case XK_Hyper_L:
        case XK_Hyper_R:
			KS_APPEND(kp, COMMAND);
            break;

        case XK_Alt_L:
        case XK_Alt_R:
			KS_APPEND(kp, OPTION);
            break;

        case XK_KP_Multiply:
			KS_APPEND(kp, KP_MULTIPLY);
            break;

        case XK_KP_Add:
			KS_APPEND(kp, KP_PLUS);
            break;

        case XK_KP_Subtract:
			KS_APPEND(kp, KP_MINUS);
            break;

        case XK_KP_Decimal:
			KS_APPEND(kp, KP_DECIMAL);
            break;

        case XK_KP_Divide:
			KS_APPEND(kp, KP_DIVIDE);
            break;
            
        case XK_KP_Enter:
			KS_APPEND(kp, KP_ENTER);
            break;

        case XK_KP_0:
        case XK_KP_1:
        case XK_KP_2:
        case XK_KP_3:
        case XK_KP_4:
        case XK_KP_5:
        case XK_KP_6:
        case XK_KP_7:
        case XK_KP_8:
        case XK_KP_9:
            KS_APPEND(kp, (keysym - XK_KP_0) + KP_0);
            break;

    }
    
	return kp;
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

keypresses raw_keypress(unsigned char c) {
	keypresses ks = { 0 };
	KS_APPEND(ks, c);
	return ks;
}

int kb_mode(session* sess) {
	int ret;
	int mode = 0;
	
	ret = ioctl(sess->fb_fd, FB_KB_MODE, &mode);
	if (ret < 0) {
		session_err(sess, "keyboard ioctl failed");
	}
	
	return mode;
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

