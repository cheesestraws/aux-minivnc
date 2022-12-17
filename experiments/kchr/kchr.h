#ifndef KCHR_H
#define KCHR_H

/* Types to make prototypes easier to read */
typedef char* kchr_t;

struct kchr_entry {
	unsigned char valid;
	unsigned char table;
	unsigned char keycode;
};

struct kchr_dead_sub {
	unsigned char valid;
	unsigned char table;
	unsigned char deadkey;
	unsigned char comp_char;
};

struct kchr_vkeypress {
	unsigned char valid;
	unsigned char modifiers;
	unsigned char keycode;
};

struct kchr_keypresses {
	struct kchr_vkeypress fst;
	struct kchr_vkeypress snd;
};

#define KCHR_NTABLES 32

struct kchr_state {
	char modifiers[KCHR_NTABLES];
	struct kchr_entry entries[256];
	struct kchr_dead_sub deads[256];
};

void kchr_load(int fb_fd, struct kchr_state* state);
struct kchr_keypresses kchr_keys_for_char(struct kchr_state* state, unsigned char c);
void kchr_print_keypress(struct kchr_vkeypress k);

int kchr_build_table_table(kchr_t kchr, unsigned char* output);
void kchr_print_modifiers(unsigned char mods);
void kchr_build_char_table(kchr_t kchr, struct kchr_entry* entries);
void kchr_build_dead_table(kchr_t kchr, struct kchr_dead_sub* entries);

#endif