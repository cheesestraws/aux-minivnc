#ifndef KCHR_H
#define KCHR_H

/* Types to make prototypes easier to read */
typedef char* kchr_t;

struct kchr_entry {
	unsigned char valid;
	unsigned char table;
	unsigned char keycode;
};

#define KCHR_NTABLES 32

int kchr_build_table_table(kchr_t kchr, unsigned char* output);
void kchr_print_modifiers(unsigned char mods);
void kchr_build_char_table(kchr_t kchr, struct kchr_entry* entries);

#endif