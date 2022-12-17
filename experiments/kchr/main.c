#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "kchr.h"
#include "../../kernel/src/fb.h"

void prentry(unsigned char c, struct kchr_entry* es) {
	struct kchr_entry e = es[c];
	char cs[2];
	
	cs[0] = c;
	cs[1] = 0;
	
	printf("%s: valid %d tab %d key %d\n", cs, e.valid, e.table, e.keycode);
}

void prdead(unsigned char c, struct kchr_dead_sub* es) {
	struct kchr_dead_sub e = es[c];
	char cs[2];
	
	cs[0] = c;
	cs[1] = 0;	

	printf("%s: valid %d tab %d key %d complete %d\n", cs, e.valid, e.table, e.deadkey, e.comp_char);
}

int main() {
	int fb_fd;
	struct kchr_keypresses keys;
	
	struct kchr_state ks;

	// open the fb
	fb_fd = open("/dev/fb0");
	if (fb_fd < 0) {
		printf("open: error %d\n", errno);
	}

	kchr_load(fb_fd, &ks);

	prentry('a', ks.entries);
	prentry('A', ks.entries);
	prentry('Z', ks.entries);
	prentry(0x8e, ks.entries);
	prentry('E', ks.entries);

	
		
	prdead(0xea, ks.deads);
	
	
	keys = kchr_keys_for_char(&ks, 0x83);
	kchr_print_keypress(keys.fst); printf(" ");
	kchr_print_keypress(keys.snd);
	printf("\n");
}