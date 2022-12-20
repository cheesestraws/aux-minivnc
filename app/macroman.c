#include "macroman.h"

/* This file maps from X keysyms to MacRoman characters. */

unsigned char keysyms_ff[256] = {
	[0x08] = 0x08,	// backspace
	[0x09] = 0x09,	// tab
	[0x0d] = 0x0d,	// return
	[0x1b] = 0x1b,	// escape
	[0xff] = 0x7f,	// delete
	
	[0x50] = 0x01,	// home
	[0x51] = 0x1c,	// left
	[0x52] = 0x1e,	// up
	[0x53] = 0x1f,	// right
	[0x54] = 0x1d,	// down
	[0x55] = 0x0b,	// pgup
	[0x56] = 0x0c,	// pgdn
	[0x57] = 0x04,	// end
	
	[0x63] = 0x05,	// insert
	[0x6a] = 0x05,	// help.  help/insert key has both labels on AEK.
};

unsigned char keysyms_00[256] = {
	[0xa0] = 0xca,	// nbsp
	[0xa1] = 0xc1,	// inverted exclamation mark
	[0xa2] = 0xa2,	// cent
	[0xa3] = 0xa3,	// pound sign
	[0xa4] = 0xdb,	// generic currency sign
	[0xa5] = 0xb4,	// yen sign
	[0xa7] = 0xa4,	// section sign
	[0xa8] = 0xac,	// diaeresis
	[0xa9] = 0xa9,	// copyright
	[0xaa] = 0xbb,	// fem. ord. indicator (superscript a)
	[0xab] = 0xc7,	// left-pointing guillemot
	[0xac] = 0xc2,	// not sign
	[0xad] = 0x2d,	// soft hyphen; rewriting to hyphen/minus ok?
	[0xae] = 0xa8,	// reg'd trademark
	[0xaf] = 0xf8,	// macron
	[0xb0] = 0xa1,	// degree sign
	[0xb1] = 0xb1,	// plus/minus sign
	[0xb4] = 0xab,	// acute accent
	[0xb5] = 0xb5,	// greek mu sign
	[0xb6] = 0xa6,	// paragraph mark
	[0xb7] = 0xe1,	// mid-dot
	[0xb8] = 0xfc,	// cedilla
	[0xba] = 0xbc,	// masc. ord. indicator (superscript o)
	[0xbb] = 0xc8,	// right-pointing guillemot
	[0xbf] = 0xc0,	// inverted question mark
	
	[0xc0] = 0xcb,	// capital a with grave
	[0xc1] = 0xe7,	// capital a with acute
	[0xc2] = 0xe5,	// capital a circumflex
	[0xc3] = 0xcc,	// capital a tilde
	[0xc4] = 0x80,	// capital a diaeresis
	[0xc5] = 0x81,	// capital a with ring
	[0xc6] = 0xae,	// capital AE ligature
	[0xc7] = 0x82,	// capital c with cedilla
	[0xc8] = 0xe9,	// capital e w/ grave
	[0xc9] = 0x83,	// capital e acute
	[0xca] = 0xe6,	// capital e circumflex
	[0xcb] = 0xe8, 	// capital e diaeresis
	[0xcc] = 0xed,	// capital I grave
	[0xcd] = 0xea,	// capital I acute
	[0xce] = 0xeb,	// capital I circumflex
	[0xcf] = 0xec,	// capital I diaeresis
	[0xd1] = 0x84,	// capital N tilde
	[0xd2] = 0xf1,	// capital O grave
	[0xd3] = 0xee, 	// capital O acute
	[0xd4] = 0xef,	// capital O circumflex
	[0xd5] = 0xcd,	// capital O tilde
	[0xd6] = 0x85,	// capital O diaeresis
	[0xd8] = 0xaf,	// capital O with a slash
	[0xd9] = 0xf4,	// capital U grave
	[0xda] = 0xf2,	// capital U acute
	[0xdb] = 0xf3,	// capital U circumflex
	[0xdc] = 0x86,	// capital U diaeresis
	[0xdf] = 0xa7,	// sharp s
	
	[0xe0] = 0x88,	// small a w/ grave
	[0xe1] = 0x87,	// small a w/ acute
	[0xe2] = 0x89,	// small a w/ circumflex
	[0xe3] = 0x8b,	// small a w/ tilde
	[0xe4] = 0x8a,	// small a w/ diaeresis
	[0xe5] = 0x8c,	// small a w/ ring above
	[0xe6] = 0xbe,	// small ae
	[0xe7] = 0x8d,	// small c w/ cedilla
	[0xe8] = 0x8f,	// small e w/ grave
	[0xe9] = 0x8e,	// small e w/ acute
	[0xea] = 0x90,	// small e w/ circumflex
	[0xeb] = 0x91,	// small e w/ diaeresis
	[0xec] = 0x93,	// small i w/ grave
	[0xed] = 0x92, 	// small i w/ acute
	[0xee] = 0x94,	// small i w/ circumflex
	[0xef] = 0x95,	// small i w/ diaeresis
	[0xf1] = 0x96,	// small n w/ tilde
	[0xf2] = 0x98,	// small o w/ grave
	[0xf3] = 0x97,	// small o w/ acute
	[0xf4] = 0x99,	// small o w/ circumflex
	[0xf5] = 0x9b,	// small o w/ tilde
	[0xf6] = 0x9a,	// small o w/ diaeresis
	[0xf7] = 0xd6,	// division sign
	[0xf8] = 0xbf,	// small o w/ stroke
	[0xf9] = 0x9d,	// small u w/ grave
	[0xfa] = 0x9c,	// small u w/ acute
	[0xfb] = 0x9e,	// small u w/ circumflex
	[0xfc] = 0x9f,	// small u w/ diaeresis
	[0xff] = 0xd8,	// small y w/ diaeresis
};

unsigned char keysym_to_macroman(int keysym) {
	/* 7 bit ascii is straight passthrough */
	if (0x20 <= keysym && keysym <= 0x7f) {
		return (unsigned char)keysym;
	}
	
	if ((keysym & 0xff00) == 0xff00) {
		return keysyms_ff[keysym & 0xff];
	}
	
	// latin-1
	if ((keysym & 0xff00) == 0x0000) {
		return keysyms_00[keysym & 0xff];
	}
	
	// loose bits
	switch (keysym) {
		case 0x0af1:	// dagger
			return 0xa0;
		case 0x0ae6:	// bullet
			return 0xa5;
		case 0x0ac9:	// trademark
			return 0xaa;
		case 0x08bd:	// not equal
			return 0xad;
		case 0x08c2:	// infinity
			return 0xb0;
		case 0x08bc:	// less than or equal to
			return 0xb2;
		case 0x08be:	// greater than or equal to
			return 0xb3;
		case 0x08ef:	// partial differential
			return 0xb6;
		case 0x07d2:	// uppercase sigma
			return 0xb7;
		case 0x07d0:	// uppercase pi
			return 0xb8;
		case 0x07f0:	// lowercase pi
			return 0xb9;
		case 0x08bf:	// integral
			return 0xba;
		case 0x07d9:	// capital omega
			return 0xbd;
		case 0x08d6:	// square root
			return 0xc3;
		case 0x08f6:	// cursive f w/ hook
			return 0xc4;
		case 0x07c4:	// capital delta
			return 0xc6;
		case 0x0aae:	// ellipsis
			return 0xc9;
		case 0x13bc:	// OE ligature
			return 0xce;
		case 0x13bd:	// oe ligature
			return 0xcf;
		case 0x0aaa:	// en dash
			return 0xd0;
		case 0x0aa9:	// em dash
			return 0xd1;
		case 0x0ad2:	// left double quote
			return 0xd2;
		case 0x0ad3:	// right double quote
			return 0xd3;
		case 0x0ad0:	// left single quote
			return 0xd4;
		case 0x0ad1:	// right single quote
			return 0xd5;
		case 0x13be:	// capital Y with diaeresis
			return 0xd9;
		case 0x0af2:	// double dagger
			return 0xe0;
		case 0x0afd:	// lowered single quote
			return 0xe2;
		case 0x0afe:	// lowered double quote
			return 0xe3;
		case 0x02b9:	// dotless i
			return 0xf5;
			
		// extended unicode keysyms:
		case 0x1002248:	// approximately equal
			return 0xc5;
		case 0x10025CA:	// lozenge
			return 0xd7;
		case 0x1002044:	// fraction slash
			return 0xda;
		case 0x1002039:	// single angular quote mark, left-pointing
			return 0xdc;
		case 0x100203a:	// single angular quote mark, right-pointing
			return 0xdd;
		case 0x100FB01:	// fi ligature
			return 0xde;
		case 0x100FB02:	// fl ligature
			return 0xdf;
		case 0x100F8FF:	// apple logo
			return 0xf0;
		case 0x10002C6:	// loose circumflex
			return 0xf6;
		case 0x10002DC:	// small tilde
			return 0xf7;
		case 0x10002D8:	// breve
			return 0xf9;
		case 0x10002D9: // dot above
			return 0xfa;
		case 0x10002da: // ring above
			return 0xfb;
		case 0x10002DD:	// double acute
			return 0xfd;
		case 0x10002DB:	// ogonek
			return 0xfe;
		case 0x10002C7: // caron
			return 0xff;
			
		// euro special case
		case 0x20ac:
			return 0xdb;
	}
	
	return 0;	
}
