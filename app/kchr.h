#ifndef KCHR_H
#define KCHR_H

#define KCHR_CMD_KEY 1
#define KCHR_SHIFT_KEY 2
#define KCHR_LOCK_KEY 4
#define KCHR_OPTION_KEY 8
#define KCHR_CTRL_KEY 16 

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

typedef struct kchr_vkeypress {
	unsigned char valid;
	unsigned char modifiers;
	unsigned char keycode;
} kchr_vkeypress;

typedef struct kchr_keypresses {
	struct kchr_vkeypress fst;
	struct kchr_vkeypress snd;
} kchr_keypresses;

#define KCHR_NTABLES 32

typedef struct kchr_state {
	char modifiers[KCHR_NTABLES];
	struct kchr_entry entries[256];
	struct kchr_dead_sub deads[256];
} kchr_state;

void kchr_load(int fb_fd, struct kchr_state* state);
struct kchr_keypresses kchr_keys_for_char(struct kchr_state* state, unsigned char c);
void kchr_print_keypress(struct kchr_vkeypress k);

int kchr_build_table_table(kchr_t kchr, unsigned char* output);
void kchr_print_modifiers(unsigned char mods);
void kchr_build_char_table(kchr_t kchr, struct kchr_entry* entries);
void kchr_build_dead_table(kchr_t kchr, struct kchr_dead_sub* entries);

#endif