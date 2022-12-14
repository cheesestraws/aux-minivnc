#ifndef KEYBOARD_MESS_H
#define KEYBOARD_MESS_H

#include "session.h"
#include "kchr.h"

typedef struct keypresses {
	char count;
	unsigned char keys[6];
} keypresses;

#define KS_APPEND(k, x) { k.keys[k.count] = (x); k.count++; }

int keysym_is_modifier(unsigned int keysym);
keypresses keysym_special_to_keypresses(unsigned int keysym);
keypresses vkey_to_keypresses(kchr_vkeypress vk);
keypresses raw_keypress(unsigned char c);

int kb_mode(session* sess);
void do_key_down(session* sess, keypresses k);
void do_key_up(session* sess, keypresses k);


#endif