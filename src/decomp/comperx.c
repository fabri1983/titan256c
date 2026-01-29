#include <types.h>
#include <sys.h>
#include "decomp/comperx.h"

#include "compressionTypesTracker.h"
#if defined(USING_COMPERX) || defined(USING_COMPERXM)

void ComperXDec_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	__asm volatile (
		"jsr ComperXDec"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","d4","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

void ComperXMDec_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	__asm volatile (
		"jsr ComperXMDec"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","d4","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

#endif // USING_COMPERX or USING_COMPERXM