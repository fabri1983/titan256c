#include "decomp/comper.h"
#include "compatibilities.h"

void comperx_dec (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	ASM_STATEMENT __volatile__ (
		"jsr ComperXDec"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","d4","cc"
	);
}

void comperxm_dec (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	ASM_STATEMENT __volatile__ (
		"jsr ComperXMDec"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","d4","cc"
	);
}