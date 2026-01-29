#include <types.h>
#include <sys.h>
#include "decomp/nibbler.h"

#include "compressionTypesTracker.h"
#ifdef USING_NIBBLER

void Denibble_caller (u8* src, u8* dest) {
// #ifdef __GNUC__
// 	register void* a0 asm ("a0") = src;
// 	register void* a1 asm ("a1") = dest;
// #else
//     u8* a0 = src;
//     u8* a1 = dest;
// #endif
// 	__asm volatile (
// 		"jsr Denibble"
// 		: "+a" (a1)
// 		: "a" (a0)
// 		: "a2","a3","a4","a5","a6","d2","d3","d4","d5","d6","d7","cc"  // backup registers used in the asm implementation, except scratch pad
// 	);
}

#endif // USING_NIBBLER