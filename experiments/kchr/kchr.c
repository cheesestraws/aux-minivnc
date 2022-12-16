#include <stdio.h>

#include "kchr.h"

/* This file includes routines for fiddling with 'KCHR' resources,
   specifically for using them to work out what ADB keycode we 
   ought to be sending. */
   
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

void kchr_build_char_table(kchr_t kchr, struct kchr_entry* entries) {
	kchr_t cursor;
	unsigned short table_count;
	int table, key_in_table;
	char character;
		
	cursor = kchr;
	
	memset(entries, 0, 256 * sizeof(struct kchr_entry));
	
	cursor += 2; // skip version
	cursor += 0x100; // skip table of tables
	
	table_count = *(short*)cursor;
	cursor += 2;
	
	printf("table_count: %d\n", table_count);
	
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

