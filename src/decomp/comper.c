#include <types.h>
#include <sys.h>
#include "decomp/comper.h"

#include "compressionTypesTracker.h"
#ifdef USING_COMPER

void ComperDec_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = src;
	register void* a1 asm ("a1") = dest;
#else
    u8* a0 = src;
    u8* a1 = dest;
#endif
	__asm volatile (
		"jsr ComperDec"
		: "+a" (a1)
		: "a" (a0)
		: "a2","d2","d3","d4","d5","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

#endif // USING_COMPER