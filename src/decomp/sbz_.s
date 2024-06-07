*======================================================================
* Copyright (C) 2024 Ian Karlsson
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute
* it freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must
*    not claim that you wrote the original software. If you use this
*    software in a product, an acknowledgment in the product
*    documentation would be appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must
*    not be misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source
*    distribution.
*======================================================================

#include "asm_mac.i"

*======================================================================
* Decompress SBZ data
*----------------------------------------------------------------------
* INPUT
*  a0 - pointer to input (compressed) data
*  a1 - pointer to output data buffer
* TRASHES
*  d0-d3/a0-a4
*======================================================================
* C prototype: extern void SBZ_decompress (u8* in, u8* out)
func SBZ_decompress
	*movem.l	4(%sp), %a0-%a1				// copy parameters into registers a0-a1
	*movem.l	%d2-%d3/%a2-%a4, -(%sp)		// save used registers (except the scratch pad)
	moveq	#0,%d2
	move.w	(%a0)+,%a2
	adda.l	%a0,%a2

	lea		.sbz_loop(%pc),%a4

	moveq	#0b00111110,%d0
	move.w	#0x8000,%d1
	jmp		(%a4)

*--------------------------------------
* long copy
* d2: ddnnnnn0
*--------------------------------------
.sbz_long:
	move.w	%d2,%d3				// 4
	and.b	%d0,%d2				// 4

	add.w	%d3,%d3				// 4
	add.w	%d3,%d3				// 4
	move.b	(%a2)+,%d3			// 8

	move.l	%a1,%a3				// 4
	suba.w	%d3,%a3				// 8

	jmp		.sbz_long_j(%pc,%d2)	// skipping 00

.sbz_extended_j:
	move.b	(%a3)+,(%a1)+
	move.b	(%a3)+,(%a1)+
	move.b	(%a3)+,(%a1)+
.sbz_long_j:
.rept 34-9
    move.b	(%a3)+,(%a1)+
.endr
.sbz_short_j:
.rept 9
    move.b	(%a3)+,(%a1)+
.endr
	jmp		(%a4)

*--------------------------------------
.sbz_direct_byte:
	move.b	(%a2)+,(%a1)+

.sbz_loop:
	add.w	%d1,%d1
	bne.s	.sbz_desc_left1
	move.w	(%a0)+,%d1
	addx.w	%d1,%d1

.sbz_desc_left1:
	bcc.s	.sbz_direct_byte

	add.w	%d1,%d1
	bne.s	.sbz_desc_left2
	move.w	(%a0)+,%d1
	addx.w	%d1,%d1

.sbz_desc_left2:
	bcc.s	.sbz_short

	move.b	(%a2)+,%d2
	add.b	%d2,%d2

	bcc.s	.sbz_long
	bpl.s	.sbz_extended
	jmp		.sbz_direct_j-128(%pc,%d2)

*--------------------------------------
* short copy
* d2: dddddnnn
*--------------------------------------
.sbz_short:
	move.b	(%a2)+,%d2
	moveq	#-8,%d3				// 4 (old asm68k compatible)
	and.b	%d2,%d3				// 4
	sub.b	%d3,%d2				// 4
	add.b	%d2,%d2				// 4

	asr.w	#3,%d3				// 10
	move.l	%a1,%a3				// 4
	adda.w	%d3,%a3				// 8  = 38
	jmp		.sbz_short_j(%pc,%d2)

*--------------------------------------
* extended copy
* d2: 0nnnnnn0 ^ 0dddddd0
*--------------------------------------
.sbz_extended:
	add.b	%d2,%d2
	move.w	(%a0)+,%d3
	eor.b	%d3,%d2
	lsr.w	#2,%d3

	move.l	%a1,%a3				// 4
	suba.w	%d3,%a3				// 8

	moveq	#34,%d3				// 259-34
	add.b	%d3,%d2
	bcs.s	.sbz_ext_skip

.sbz_ext_loop:
.rept 34
    move.b	(%a3)+,(%a1)+		// 12*2 + (12*n) min 24
.endr
	add.b	%d3,%d2
	bcc.s	.sbz_ext_loop
.sbz_ext_skip:
	add.b	%d2,%d2
	jmp		.sbz_extended_j-.sbz_loop(%a4,%d2)

.sbz_direct_j:
	*movem.l	(sp)+, %d2-%d3/%a2-%a4	// restore registers (except the scratch pad)
	rts
.rept 71
    move.b	(%a2)+,(%a1)+
.endr
	jmp		(%a4)