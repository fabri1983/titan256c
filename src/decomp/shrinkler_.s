* Copyright 1999-2022 Aske Simon Christensen.
*
* The code herein is free to use, in whole or in part,
* modified or as is, for any legal purpose.
*
* No warranties of any kind are given as to its behavior
* or suitability.

.equ    INIT_ONE_PROB, 0x8000
.equ    ADJUST_SHIFT, 4
.equ    SINGLE_BIT_CONTEXTS, 1
.equ    NUM_CONTEXTS, 1536

* If compressed with --bytes / -b option then set 0, else 1
.equ    FLAG_PARITY_CONTEXT, 0

#include "asm_mac.i"

* Decompress Shrinkler-compressed data produced with the --data option.
*
* A0 = Compressed data
* A1 = Decompressed data destination
*
* Uses additional 3 kilobytes of space from stack
* Preserves D2-D6/A5-A6.
*
* Decompression code may read one byte beyond compressed data.
* The contents of this byte does not matter.

* C prototype: extern void ShrinklerDecompress (u8* in, u8* out)
func ShrinklerDecompress
    movem.l 4(sp),a0-a1
	movem.l	d2-d6/a5-a6,-(sp)

	move.l	a1,a5
	move.l	a1,a6

	* Init range decoder state
	moveq	#0,d2
	moveq	#1,d3
	moveq	#-0x80,d4

	* Init probabilities
	move.l	#(NUM_CONTEXTS/2)-1,d6
.init:
	move.l	#(INIT_ONE_PROB<<16 | INIT_ONE_PROB),-(sp)
	dbf     d6,.init
    moveq   #0,d6

	* D6 = 0
.lit:
	* Literal
	addq.b	#1,d6
.getlit:
	bsr.s	GetBit
	addx.b	d6,d6
	bcc.s	.getlit
	move.b	d6,(a5)+
.switch:
	* After literal
	bsr.s	GetKind
	bcc.s	.lit
	* Reference
	moveq	#-1,d6
	bsr.s	GetBit
	bcc.s	.readoffset
.readlength:
	moveq	#4,d6
	bsr.s	GetNumber
.copyloop:
	move.b	(a5,d5.w),(a5)+
	subq.w	#1,d0
	bne.s	.copyloop
	* After reference
	bsr.s	GetKind
	bcc.s	.lit
.readoffset:
	moveq	#3,d6
	bsr.s	GetNumber
	moveq	#2,d5
	sub.w	d0,d5
	bne.s	.readlength

	lea.l	NUM_CONTEXTS*2(sp),sp
	movem.l	(sp)+,d2-d6/a5-a6
	rts

GetKind:
	* Use parity as context
.if FLAG_PARITY_CONTEXT == 0
    moveq   #0,d6
.endif
.if FLAG_PARITY_CONTEXT == 1
    move.w	a5,d6
    and.w	#1,d6
    lsl.w	#8,d6
.endif
	bra.s	GetBit

GetNumber:
	* D6 = Number context

	* Out: Number in D0
	lsl.w	#8,d6
.numberloop:
	addq.b	#2,d6
	bsr.s	GetBit
	bcs.s	.numberloop
	moveq	#1,d0
	subq.b	#1,d6
.bitsloop:
	bsr.s	GetBit
	addx.w	d0,d0
	subq.b	#2,d6
	bcc.s	.bitsloop
	rts

	* D6 = Bit context

	* D2 = Range value
	* D3 = Interval size
	* D4 = Input bit buffer

	* Out: Bit in C and X

readbit:
	add.b	d4,d4
	bne.s	nonewword
	move.b	(a0)+,d4
	addx.b	d4,d4
nonewword:
	addx.w	d2,d2
	add.w	d3,d3
GetBit:
	tst.w	d3
	bpl.s	readbit

	lea.l	4+SINGLE_BIT_CONTEXTS*2(sp,d6.w),a1
	add.l	d6,a1
	move.w	(a1),d1
	* D1 = One prob

	lsr.w	#ADJUST_SHIFT,d1
	sub.w	d1,(a1)
	add.w	(a1),d1

	mulu.w	d3,d1
	swap.w	d1

	sub.w	d1,d2
	blo.s	.one
.zero:
	* oneprob = oneprob * (1 - adjust) = oneprob - oneprob * adjust
	sub.w	d1,d3
	* 0 in C and X
	rts
.one:
	* onebrob = 1 - (1 - oneprob) * (1 - adjust) = oneprob - oneprob * adjust + adjust
	add.w	#(0xffff>>ADJUST_SHIFT),(a1)
	move.w	d1,d3
	add.w	d1,d2
	* 1 in C and X
	rts