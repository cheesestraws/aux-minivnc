#ifndef KEYBOARD_MESS_H
#define KEYBOARD_MESS_H

#include "session.h"

typedef struct keypresses {
	char count;
	unsigned char keys[3];
} keypresses;

keypresses sym_to_keypresses(unsigned int keysym);

void do_key_down(session* sess, keypresses k);
void do_key_up(session* sess, keypresses k);


#endif