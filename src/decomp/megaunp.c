#include "decomp/megaunp.h"
#include "compatibilities.h"

#include "compressionTypesTracker.h"
#ifdef USING_MEGAPACK

void init_mega_caller () {
// #ifdef __GNUC__
//     register vu8* a2 asm ("a2") = (vu8*)0xFFD4CA;
// #else
//     vu8* a2 =  (vu8*)0xFFD4CA;
// #endif
//     __asm volatile (
//         "jsr init_mega"
//         : "+a" (a2)
//         : "a" (a2)
//         : "d2", "d3", "d4", "d5", "d6", "d7", "a3", "a4", "a5", "a6"  // backup registers used in the asm implementation, except scratch pad
//     );
}

void megaunp_caller (const u8* data, u8* dest) {
// #ifdef __GNUC__
//     register const u8* a0 asm ("a0") = data;
//     register u8* a1 asm ("a1") = dest;
// #else
//     u8* a0 = data;
//     u8* a1 = dest;
// #endif
//     __asm volatile (
//         "jsr megaunp"
//         : "+a" (a0), "+a" (a1)
//         : "a" (a0), "a" (a1)
//         : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a3", "a4", "a5", "a6"  // backup registers used in the asm implementation, except scratch pad
//     );
}

#endif // USING_MEGAPACK