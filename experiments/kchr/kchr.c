#include <stdio.h>
#include <fcntl.h>

#include "kchr.h"
#include "../../kernel/src/fb.h"

/* A KCHR is a resource that maps keyboard keys to the characters
   they should produce.  They represent a "keyboard layout".
   
   This file contains routines to take a kchr resource and to
   reverse it, to turn it into something we can interrogate to
   start from a character and end up with some key codes. */
   
// Builds a table number => modifiers map, which is the inverse
// of the modifiers => table number map in the reesource
int kchr_build_table_table(kchr_t kchr, unsigned char* output) {
	kchr_t cursor;
	int i;
	
	cursor = kchr + 2; // +2 to skip over version field
	
	// There are 0x80 entries in the modifier->table map
	memset(output, 0xff, KCHR_NTABLES);
	for (i = 0; i < 0x80; i++) {
		// i is the modifiers.  cursor[i] is the table number.

		if (cursor[i] >= KCHR_NTABLES) {
			// too many tables
			return -1;
		}
		
		// Have we found a simpler route to this table?
		if (output[cursor[i]] == 0xff) {
			// Nope!  So let's index it.
			output[cursor[i]] = i;
		}
	}
}

void kchr_print_modifiers(unsigned char mods) {
	int i;
	int first;
	first = 1;
	
	for (i = 0; i < 8; i++) {
		if ((mods & (1 << i)) == 0) {
			continue;
		}
		
		if (!first) {
			printf("+");
		}

		switch(i) {
		case 0:
			printf("cmd"); break;
		case 1:
			printf("shift"); break;
		case 2:
			printf("lock"); break;
		case 3:
			printf("opt"); break;
		case 4:
			printf("ctrl"); break;
		default:
			printf("unimpl"); break;
		}
		
		first = 0;
	}
}

// Builds a char => (table, keycode) table, which is the inverse of the
// (table, keycode) => char table in the resource.
void kchr_build_char_table(kchr_t kchr, struct kchr_entry* entries) {
	kchr_t cursor;
	unsigned short table_count;
	int table, key_in_table;
	unsigned char character;
		
	cursor = kchr;
	
	memset(entries, 0, 256 * sizeof(struct kchr_entry));
		
	cursor += 2; // skip version
	cursor += 0x100; // skip table of tables
	
	table_count = *(short*)cursor;
	cursor += 2;
		
	for (table = 0; table < table_count; table++) {
		for (key_in_table = 0; key_in_table < 0x80; key_in_table++) {
			character = cursor[key_in_table];
			
			if (entries[character].valid == 0) {
				entries[character].valid = 1;
				entries[character].table = table;
				entries[character].keycode = key_in_table;
			}
		}
		cursor += 0x80;
	}
}

// Builds char => deadkey tables, which are the inverse of the
// deadkey => char tables in the resource.
void kchr_build_dead_table(kchr_t kchr, struct kchr_dead_sub* entries) {
	kchr_t cursor;
	unsigned short table_count;
	unsigned short dead_count;
	int i, j;
	
	unsigned char dead_table;
	unsigned char dead_keycode;
	unsigned short completor_count;
	unsigned char completor_char;
	unsigned char substituting_char;

	cursor = kchr;
	
	memset(entries, 0, 256 * sizeof(struct kchr_dead_sub));
	
	cursor += 2; // skip version
	cursor += 0x100; // skip table of tables
	
	table_count = *(short*)cursor; // get and skip table count
	cursor += 2;
	
	cursor += table_count * 0x80; // skip keytables
	
	// now the cursor points to the beginning of deadkey stuff
	
	dead_count = *(short*)cursor; // how many dead key tables?
	cursor += 2;
	
	for (i = 0; i < dead_count; i++) {	
		// here we follow the resource definition in systypes.r
		dead_table = *cursor;
		cursor++;
		dead_keycode = *cursor;
		cursor++;
		
		completor_count = *(short*)cursor;
		cursor += 2;
				
		for (j = 0; j < completor_count; j++) {
			completor_char = *cursor;
			cursor++;
			substituting_char = *cursor;
			cursor++;
						
			if (entries[substituting_char].valid == 0) {
				// we have no way of reaching this substituting_char
				// so far
								
				entries[substituting_char].valid = 1;
				entries[substituting_char].table = dead_table;
				entries[substituting_char].deadkey = dead_keycode;
				entries[substituting_char].comp_char = completor_char;
			}
		}
		
		cursor += 2; // we don't care much about no match char
	}
}

void kchr_load(int fb_fd, struct kchr_state* state) {
	unsigned char kchr[3072];
	char* dst;
	int i;
	struct fb_kchr_chunk chunk;

	// grab the kchr
	dst = kchr;
	for (i = 0; i < 32; i++) {
		chunk.chunk = i;
		ioctl(fb_fd, FB_KB_KCHR_CHUNK, &chunk);
		memcpy(dst, chunk.data, 96);
		dst += 96;
	}
	
	kchr_build_table_table(kchr, state->modifiers);
	kchr_build_char_table(kchr, state->entries);
	kchr_build_dead_table(kchr, state->deads);
}

void kchr_print_keypress(struct kchr_vkeypress k) {
	if (!k.valid) {
		return;
	}
	
	kchr_print_modifiers(k.modifiers);
	if (k.modifiers) {
		printf("+");
	}
	printf("#0x%02x", (int)k.keycode);
}

struct kchr_keypresses kchr_keys_for_char(struct kchr_state* state, unsigned char c) {
	struct kchr_keypresses keys = {0};
	unsigned char snd_chr;
	
	// Easy case: is it a single keystroke?
	if (state->entries[c].valid != 0) {
		// Yup!
		keys.fst.valid = 1;
		keys.fst.modifiers = state->modifiers[state->entries[c].table];
		keys.fst.keycode = state->entries[c].keycode;
		return keys;
	}
	
	// If not, is it a dead key?
	if (state->deads[c].valid != 0) {
		// The first keypress is the dead key press
		keys.fst.valid = 1;
		keys.fst.modifiers = state->modifiers[state->deads[c].table];
		keys.fst.keycode = state->deads[c].deadkey;
		
		// The second keypress is given by the completion char: we need
		// to run this through the kchr index *again* to get a keycode
		snd_chr = state->deads[c].comp_char;
		
		keys.snd.valid = 1;
		keys.snd.modifiers = state->modifiers[state->entries[snd_chr].table];
		keys.snd.keycode = state->entries[snd_chr].keycode;
		
		return keys;
	}
	
	// otherwise just return our empty keypresses
	return keys;
}