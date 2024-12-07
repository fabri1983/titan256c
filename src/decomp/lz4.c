#include "decomp/lz4.h"
#include "compatibilities.h"

#include "compressionTypesTracker.h"
#ifdef USING_LZ4

void lz4_normal_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	ASM_STATEMENT volatile (
		"jsr lz4_frame_depack_normal"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","d4","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

void lz4_fastest_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	ASM_STATEMENT volatile (
		"jsr lz4_frame_depack_fastest"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","d5","d7","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

#endif // USING_LZ4