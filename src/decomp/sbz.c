//=====================================================================
// This is a C API for the 68k version of the decompressor.
// Feel free to modify it to fit your needs.
//=====================================================================
// Copyright (C) 2024 Ian Karlsson
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any
// purpose, including commercial applications, and to alter it and
// redistribute it freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must
//    not claim that you wrote the original software. If you use this
//    software in a product, an acknowledgment in the product
//    documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must
//    not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source
//    distribution.
//=====================================================================

#include "decomp/sbz.h"
#include "compatibilities.h"

#include "compressionTypesTracker.h"
#ifdef USING_SBZ

u8* SBZ_blob_decompress(const u8* in, u8* out)
{
	/*
		move.w    #0x3458, d5
		addq.w    #1, d5
		sub.w     d5, d0
		moveq     #0x6a, d1
		move.b    d0, (a1)+
		move.w    #0x3e, d0
		move.b    d0, (a1)+
		dbra      d1, 0x8000
		movem.w   d2-d3/a0-a1, -(a7)
		moveq     #0x2, d2
		move.b    d2, d6
		moveq     #0x43, d2
		move.b    d2, (a1)+
		move.b    d2, (a1)+
		addq.w    #1, d2
		bra       0x2649
		cmpi.w    #0xfb, (a0)+
		bne.s     0x2008
		moveq     #0xdb, d1
		dbra      d1, 0x12db
		moveq     #0x4e, d1
		addq.w    #1, d1
		moveq     #0xda, d2
		subi.w    #0x41, d2
		sub.w     d2, d0
		moveq     #0x18, d2
		move.b    (a0)+, d2
		addq.b    #1, d2
		sub.w     d2, d0
		bra       0x3218
		subi.w    #0x402, d4
		subq.w    #1, d4
		move.w    d4, d6
		bra       0x640c
		move.w    d6, (a1)+
		subq.w    #1, d2
		bra       0x141a
		move.w    #0x76f8, d6
		subq.w    #1, d1
		bne       0x9403
		move.w    d6, (a1)+
		addq.w    #1, d1
		bra       0x2649
		move.w    d6, (a1)+
		addq.w    #1, d1
		bra       0x7604
		dbra      d1, 0x12db
		moveq     #0x403, d1
		sub.w     d1, d0
	*/
	static const u16 asm_blob[] = {
		0x7400,0x3458,0xd5c8,0x49fa,0x006a,0x703e,0x323c,0x8000,
		0x4ed4,0x3602,0xc400,0xd643,0xd643,0x161a,0x2649,0x96c3,
		0x4efb,0x2008,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x4ed4,
		0x12da,0xd241,0x6604,0x3218,0xd341,0x64f4,0xd241,0x6604,
		0x3218,0xd341,0x640c,0x141a,0xd402,0x6486,0x6a18,0x4efb,
		0x20f6,0x141a,0x76f8,0xc602,0x9403,0xd402,0xe643,0x2649,
		0xd6c3,0x4efb,0x20b8,0xd402,0x3618,0xb702,0xe44b,0x2649,
		0x96c3,0x7622,0xd403,0x6548,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,
		0x12db,0x12db,0x12db,0x12db,0x12db,0x12db,0xd403,0x64b8,
		0xd402,0x4ef4,0x20b2,0x4e75,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,0x12da,
		0x12da,0x12da,0x12da,0x4ed4
	};

#ifdef __GNUC__
	register const void* a0 asm ("a0") = in;
	register unsigned char* a1 asm ("a1") = out;
#else
    u8* a0 = in;
    u8* a1 = out;
#endif

	ASM_STATEMENT volatile (
		"jsr %2;"
		: "+a" (a1)
		: "a" (a0), "m" (asm_blob)
		: "a2","a3","a4","d0","d1","d2","d3","cc"  // backup registers used in the asm implementation, except scratch pad
	);

	return a1; // return end of decompressed data
}

void SBZ_decompress_caller (u8* in, u8* out) {
#ifdef __GNUC__
	register void* a0 asm ("a0") = in;
	register void* a1 asm ("a1") = out;
#else
    u8* a0 = in;
    u8* a1 = out;
#endif
	ASM_STATEMENT volatile (
		"jsr SBZ_decompress;"
		: "+a" (a1)
		: "a" (a0)
		: "a2","a3","a4","d2","d3","cc"  // backup registers used in the asm implementation, except scratch pad
	);
}

#endif // USING_SBZ