#include "decomp/bytekiller.h"
#include "compatibilities.h"

#include "compressionTypesTracker.h"
#ifdef USING_BYTEKILLER

void bytekiller_depack_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	ASM_STATEMENT __volatile__ (
		"jsr bytekiller_depack\n"
		: "+a" (a1)
		: "a" (a0)
		: "a2","d2","d3","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

#endif // USING_BYTEKILLER