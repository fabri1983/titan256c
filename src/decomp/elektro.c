#include "decomp/elektro.h"
#include "compatibilities.h"

#include "compressionTypesTracker.h"
#ifdef USING_ELEKTRO

void elektro_unpack_caller (u8* src, u8* dest) {
#ifdef __GNUC__
	register void* a1 asm ("a1") = src;
	register void* a2 asm ("a2") = dest;
#else
    u8* a1 = src;
    u8* a2 = dest;
#endif
	ASM_STATEMENT volatile (
		"jsr elektro_unpack"
		: "+a" (a2)
		: "a" (a1)
		: "cc"   // Registers are already saved in the asm implementaiton and can't be modified without affecting the jump index tables
	);
}

#endif // USING_ELEKTRO