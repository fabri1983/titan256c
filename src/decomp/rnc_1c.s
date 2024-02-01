
*------------------------------------------------------------------------------
* PRO-PACK Unpack Source Code (Compact Version) - MC68000, Method 1
*
* Copyright (c) 1991,92 Rob Northen Computing, U.K. All Rights Reserved.
*
* File: RNC_1C.S
*
* Date: 24.3.92
*------------------------------------------------------------------------------

#include "asm_mac.i"

*------------------------------------------------------------------------------
* Equates
*------------------------------------------------------------------------------

#define	HEADER_LEN 18
#define	RAW_TABLE 0
#define	POS_TABLE RAW_TABLE+16*8
#define	LEN_TABLE POS_TABLE+16*8
#define	BUFSIZE 16*8*3

#define	counts %d4
#define	key %d5
#define	bit_buffer %d6
#define	bit_count %d7

#define	input %a3
#define	output %a4
#define	output_hi %a5

*------------------------------------------------------------------------------
* Macros
*------------------------------------------------------------------------------

.macro getrawREP_rnc1
6:
		move.b	(input)+,(output)+
		dbra	%d0,6b
.endm

*------------------------------------------------------------------------------
* PRO-PACK Unpack Routine (Compact Version) - MC68000, Method 1
*
* on entry,
*	a0.l = start address of packed file
*	a1.l = start address to unpack file
*	(note: a1 cannot be equal to a0)
*	stack space required: $1DC bytes
*
*	all other registers are preserved
*------------------------------------------------------------------------------

* C prototype: u32 rnc1_Unpack (u8 *src, u8 *dest);
func rnc1_Unpack
		movem.l 4(%sp), %a0-%a1			// copy parameters into registers a0-a1
		movem.l	%d2-%d7/%a2-%a5,-(%sp)	// save registers (except the scratch pad)
		lea	-BUFSIZE(%sp),%sp
		move.l	%sp,%a2
		addq.w	#4,%a0
		bsr	rnc1_read_long
		lea	HEADER_LEN-8(%a0),input
		move.l	%a1,output
		lea	(output,%d0.l),output_hi

		moveq.l	#0,bit_count
		move.b	1(input),bit_buffer
		rol.w	#8,bit_buffer
		move.b	(input),bit_buffer
		moveq.l	#2,%d0
		moveq.l	#2,%d1
		bsr	rnc1_input_bits
rnc1_unpack2:
		move.l	%a2,%a0
		bsr	rnc1_make_huftable
		lea	POS_TABLE(%a2),%a0
		bsr	rnc1_make_huftable
		lea	LEN_TABLE(%a2),%a0
		bsr	rnc1_make_huftable
		moveq.l	#-1,%d0
		moveq.l	#16,%d1
		bsr	rnc1_input_bits
		move.w	%d0,counts
		subq.w	#1,counts
		bra.s	rnc1_unpack5		
rnc1_unpack3:
		lea	POS_TABLE(%a2),%a0
		moveq.l	#0,%d0
		bsr.s	rnc1_input_value
		neg.l	%d0
		lea	-1(output,%d0.l),%a1
		lea	LEN_TABLE(%a2),%a0
		bsr.s	rnc1_input_value
		move.b	(%a1)+,(output)+
rnc1_unpack4:
		move.b	(%a1)+,(output)+
		dbra	%d0,rnc1_unpack4
rnc1_unpack5:
		move.l	%a2,%a0
		bsr.s	rnc1_input_value
		subq.w	#1,%d0
		bmi.s	rnc1_unpack6
		getrawREP_rnc1
		move.b	1(input),%d0
		rol.w	#8,%d0
		move.b	(input),%d0
		lsl.l	bit_count,%d0
		moveq.l #1,%d1
		lsl.w	bit_count,%d1
		subq.w	#1,%d1
		and.l	%d1,bit_buffer
		or.l	%d0,bit_buffer
rnc1_unpack6:
		dbra	counts,rnc1_unpack3
		cmp.l	output_hi,output
		bcs.s	rnc1_unpack2

		lea	BUFSIZE(%sp),%sp
		movem.l	(%sp)+,%d2-%d7/%a2-%a5		// restore registers (except the scratch pad)
		rts

rnc1_input_value:
		move.w	(%a0)+,%d0
		and.w	bit_buffer,%d0
		sub.w	(%a0)+,%d0
		bne.s	rnc1_input_value
		move.b	16*4-4(%a0),%d1
		sub.b	%d1,bit_count
		bge.s	rnc1_input_value2
		bsr.s	rnc1_input_bits3
rnc1_input_value2:
		lsr.l	%d1,bit_buffer
		move.b	16*4-3(%a0),%d0
		cmp.b	#2,%d0
		blt.s	rnc1_input_value4
		subq.b	#1,%d0
		move.b	%d0,%d1
		move.b	%d0,%d2
		move.w	16*4-2(%a0),%d0
		and.w	bit_buffer,%d0
		sub.b	%d1,bit_count
		bge.s	rnc1_input_value3
		bsr.s	rnc1_input_bits3
rnc1_input_value3:
		lsr.l	%d1,bit_buffer
		bset	%d2,%d0
rnc1_input_value4:
		rts

rnc1_input_bits:
		and.w	bit_buffer,%d0
		sub.b	%d1,bit_count
		bge.s	rnc1_input_bits2
		bsr.s	rnc1_input_bits3
rnc1_input_bits2:
		lsr.l	%d1,bit_buffer
		rts

rnc1_input_bits3:
		add.b	%d1,bit_count
		lsr.l	bit_count,bit_buffer
		swap	bit_buffer
		addq.w	#4,input
		move.b	-(input),bit_buffer
		rol.w	#8,bit_buffer
		move.b	-(input),bit_buffer
		swap	bit_buffer
		sub.b	bit_count,%d1
		moveq.l	#16,bit_count
		sub.b	%d1,bit_count
		rts

rnc1_read_long:
		moveq.l	#3,%d1
rnc1_read_long2:
		lsl.l	#8,%d0
		move.b	(%a0)+,%d0
		dbra	%d1,rnc1_read_long2
		rts

rnc1_make_huftable:
		moveq.l	#0x1F,%d0
		moveq.l	#5,%d1
		bsr.s	rnc1_input_bits
		subq.w	#1,%d0
		bmi.s	rnc1_make_huftable8
		move.w	%d0,%d2
		move.w	%d0,%d3
		lea	-16(%sp),%sp
		move.l	%sp,%a1
rnc1_make_huftable3:
		moveq.l	#0xF,%d0
		moveq.l	#4,%d1
		bsr.s	rnc1_input_bits
		move.b	%d0,(%a1)+
		dbra	%d2,rnc1_make_huftable3
		moveq.l	#1,%d0
		ror.l	#1,%d0
		moveq.l	#1,%d1
		moveq.l	#0,%d2
		movem.l	%d5-%d7,-(%sp)
rnc1_make_huftable4:
		move.w	%d3,%d4
		lea	12(%sp),%a1
rnc1_make_huftable5:
		cmp.b	(%a1)+,%d1
		bne.s	rnc1_make_huftable7
		moveq.l	#1,%d5
		lsl.w	%d1,%d5
		subq.w	#1,%d5
		move.w	%d5,(%a0)+
		move.l	%d2,%d5
		swap	%d5
		move.w	%d1,%d7
		subq.w	#1,%d7
rnc1_make_huftable6:
		roxl.w	#1,%d5
		roxr.w	#1,%d6
		dbra	%d7,rnc1_make_huftable6
		moveq.l	#16,%d5
		sub.b	%d1,%d5
		lsr.w	%d5,%d6
		move.w	%d6,(%a0)+
		move.b	%d1,16*4-4(%a0)
		move.b	%d3,%d5
		sub.b	%d4,%d5
		move.b	%d5,16*4-3(%a0)
		moveq.l	#1,%d6
		subq.b	#1,%d5
		lsl.w	%d5,%d6
		subq.w	#1,%d6
		move.w	%d6,16*4-2(%a0)
		add.l	%d0,%d2
rnc1_make_huftable7:
		dbra	%d4,rnc1_make_huftable5
		lsr.l	#1,%d0
		addq.b	#1,%d1
		cmp.b	#17,%d1
		bne.s	rnc1_make_huftable4
		movem.l	(%sp)+,%d5-%d7
		lea	16(%sp),%sp
rnc1_make_huftable8:
		rts
