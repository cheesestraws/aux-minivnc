#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "kchr.h"
#include "../../kernel/src/fb.h"

void prentry(char c, struct kchr_entry* es) {
	struct kchr_entry e = es[c];
	char cs[2];
	
	cs[0] = c;
	cs[1] = 0;
	
	printf("%s: valid %d tab %d key %d\n", cs, e.valid, e.table, e.keycode);
}

int main() {
	int fb_fd;
	unsigned char kchr[3072];
	unsigned char tablemods[32];
	struct kchr_entry entries[256];
	char* dst;
	int i;
	struct fb_kchr_chunk chunk;

	// open the fb
	fb_fd = open("/dev/fb0");
	if (fb_fd < 0) {
		printf("open: error %d\n", errno);
	}

	// grab the kchr
	dst = kchr;
	for (i = 0; i < 32; i++) {
		chunk.chunk = i;
		ioctl(fb_fd, FB_KB_KCHR_CHUNK, &chunk);
		memcpy(dst, chunk.data, 96);
		dst += 96;
	}

	kchr_build_table_table(kchr, tablemods);

	for (i = 0; i < 32; i++) {
		printf("%d => ", i, (int)(tablemods[i]));
		kchr_print_modifiers(tablemods[i]);
		printf("\n");
	}
	printf("\n");

	
	kchr_build_char_table(kchr, entries);
	
	prentry('a', entries);
	prentry('A', entries);
	prentry('Z', entries);
	prentry(0x8e, entries);

	
}