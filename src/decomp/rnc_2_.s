*------------------------------------------------------------------------------
* PRO-PACK Unpack Source Code - MC68000, Method 2
*
* Copyright (c) 1991,92 Rob Northen Computing, U.K. All Rights Reserved.
*
* File: RNC_2.S
*
* Date: 24.3.92
*------------------------------------------------------------------------------

#include "asm_mac.i"

*------------------------------------------------------------------------------
* Conditional Assembly Flags
*------------------------------------------------------------------------------

#define CHECKSUMS 0		// set this flag to 1 if you require
						// the data to be validated

#define PROTECTED 0		// set this flag to 1 if you are unpacking
						// a file packed with option "K"

*------------------------------------------------------------------------------
* Return Codes
*------------------------------------------------------------------------------

#define NOT_PACKED 0
#define PACKED_CRC -1
#define UNPACKED_CRC -2

*------------------------------------------------------------------------------
* Other Equates
*------------------------------------------------------------------------------

#define PACK_TYPE 2
#define PACK_ID ('R'<<24)|('N'<<16)|('C'<<8)|PACK_TYPE
#define HEADER_LEN 18
#define CRC_POLY 0xA001

#define input %a3
#define input_hi %a4
#define output %a5
#define output_hi %a6

#define key %d4
#define len %d5
#define pos %d6
#define bitbuf %d7

*------------------------------------------------------------------------------
* Macros
*------------------------------------------------------------------------------

.macro getbit
		add.b	bitbuf,bitbuf
.endm

.macro reload
		move.b	(input)+,bitbuf
		addx.b	bitbuf,bitbuf
.endm

.macro getraw
#if PROTECTED == 0
		move.b	(input)+,(output)+
#else
		move.b	(input)+,%d0
		eor.b	key,%d0
		move.b	%d0,(output)+
		ror.w	#1,key
#endif
.endm

.macro getrawREP
7:
#if PROTECTED == 0
		move.b	(input)+,(output)+
		move.b	(input)+,(output)+
		move.b	(input)+,(output)+
		move.b	(input)+,(output)+
#else
		move.b	(input)+,%d0
		eor.b	key,d0
		move.b	%d0,(output)+
		move.b	(input)+,%d0
		eor.b	key,%d0
		move.b	%d0,(output)+
		move.b	(input)+,%d0
		eor.b	key,%d0
		move.b	%d0,(output)+
		move.b	(input)+,%d0
		eor.b	key,%d0
		move.b	%d0,(output)+
#endif
		dbra	pos,7b
#if PROTECTED == 1
		ror.w	#1,key
#endif
.endm

*------------------------------------------------------------------------------
* PRO-PACK Unpack Routine - MC68000, Method 2
*
* on entry,
*	d0.l = packed data key, or 0 if file was not packed with a key
*	a0.l = start address of packed file
*	a1.l = start address to write unpacked file
* on exit,
*	d0.l = length of unpacked file in bytes OR error code
*		 0 = not a packed file
*		-1 = packed data CRC error
*		-2 = unpacked data CRC error
*
*	all other registers are preserved
*------------------------------------------------------------------------------
* C prototype: extern u32 rnc2_Unpack (u16 key, u8 *src, u8 *dest);
func rnc2_Unpack
		movem.l 4(%sp), d0/%a0-%a1		// copy parameters into registers d0/a0-a1
		movem.l	%d1-%d7/%a2-%a6,-(sp)   // save registers (except the scratch pad)

#if PROTECTED == 1
		move.l	%d0,key
#endif

		bsr		read_long
		moveq.l	#NOT_PACKED,%d1
		cmp.l	#PACK_ID,%d0
		bne		unpack10
		bsr		read_long
		move.l	%d0,(%sp)
		lea		HEADER_LEN-8(%a0),input
		move.l	%a1,output
		lea		(output,%d0.l),output_hi
		bsr		read_long
		lea		(input,%d0.l),input_hi

#if CHECKSUMS == 1
		move.l	input,%a1
		bsr		crc_block
		lea		-6(input),%a0
		bsr		read_long
		moveq.l	#PACKED_CRC,%d1
		cmp.w	%d2,%d0
		bne		unpack10
		swap	%d0
		move.w	%d0,-(%sp)
#endif

		clr.w	-(%sp)
		cmp.l	input_hi,output
		bcc.s	unpack7
		moveq.l	#0,%d0
		move.b	-2(input),%d0
		lea		(output_hi,%d0.l),%a0
		cmp.l	input_hi,%a0
		bls.s	unpack7
		addq.w	#2,%sp

		move.l	input_hi,%d0
		btst	#0,%d0
		beq.s	unpack2
		addq.w	#1,input_hi
		addq.w	#1,%a0
unpack2:
		move.l	%a0,%d0
		btst	#0,%d0
		beq.s	unpack3
		addq.w	#1,%a0
unpack3:
		moveq.l	#0,%d0
unpack4:
		cmp.l	%a0,output_hi
		beq.s	unpack5
		move.b	-(%a0),%d1
		move.w	%d1,-(%sp)
		addq.b	#1,%d0
		bra.s	unpack4
unpack5:
		move.w	%d0,-(%sp)
		add.l	%d0,%a0
#if PROTECTED == 1
		move.w	key,-(%sp)
#endif
unpack6:
		lea		-8*4(input_hi),input_hi
		movem.l	(input_hi),%d0-%d7
		movem.l	%d0-%d7,-(%a0)
		cmp.l	input,input_hi
		bhi.s	unpack6
		sub.l	input_hi,input
		add.l	%a0,input
#if PROTECTED == 1
		move.w	(%sp)+,key
#endif

unpack7:	
		moveq	#-0x80,bitbuf
		add.b	bitbuf,bitbuf
		reload
		getbit
		bra		xLoop

Fetch0:
		reload
		bra.s	Back0
Fetch1:
		reload
		bra.s	Back1
Fetch2:
		reload
		bra.s	Back2
Fetch3:
		reload
		bra.s	Back3
Fetch4:
		reload
		bra.s	Back4
Fetch5:
		reload
		bra.s	Back5
Fetch6:
		reload
		bra.s	Back6
Fetch7:
		reload
		bra.s	Back7

Raw:
		moveq	#3,len
x4Bits:
		add.b	bitbuf,bitbuf
		beq.s	Fetch0
Back0:
		addx.w	pos,pos
		dbra	len,x4Bits
		addq.w	#2,pos
		getrawREP
		bra.s	xLoop

GetLen:
		getbit
		beq.s	Fetch1
Back1:
		addx.w	len,len
		getbit
		beq.s	Fetch2
Back2:
		bcc.s	Copy
		subq.w	#1,len
		getbit
		beq.s	Fetch3
Back3:
		addx.w	len,len

		cmp.b	#9,len
		beq.s	Raw

Copy:
		getbit
		beq.s	Fetch4
Back4:
		bcc.s	ByteDisp2
		getbit
		beq.s	Fetch5
Back5:
		addx.w	pos,pos
		getbit
		beq.s	Fetch6
Back6:
		bcs.s	BigDisp
		tst.w	pos
		bne.s	ByteDisp
		addq.w	#1,pos
Another:
		getbit
		beq.s	Fetch7
Back7:
		addx.w	pos,pos

ByteDisp:
		rol.w	#8,pos
ByteDisp2:
		move.b	(input)+,pos
		move.l	output,%a0
		sub.w	pos,%a0
		subq.w	#1,%a0
		lsr.w	#1,len
		bcc.s	ByteDisp3
		move.b	(%a0)+,(output)+
ByteDisp3:
		subq.w	#1,len
		tst.w	pos
		bne.s	ByteDisp5
		move.b	(%a0),pos
ByteDisp4:
		move.b	pos,(output)+
		move.b	pos,(output)+
		dbra	len,ByteDisp4
		bra.s	xLoop
ByteDisp5:
		move.b	(%a0)+,(output)+
		move.b	(%a0)+,(output)+
		dbra	len,ByteDisp5
		bra.s	xLoop

GetBits:
		reload
		bcs.s	String
xByte:
		getraw
xLoop:
		getbit
		bcs.s	Chkz
		getraw
		getbit
		bcc.s	xByte
Chkz:
		beq.s	GetBits

String:
		moveq	#2,len
		moveq	#0,pos
		getbit
		beq.s	Fetch8
Back8:
		bcc		GetLen

Smalls:
		getbit
		beq.s	Fetch9
Back9:
		bcc.s	ByteDisp2
		addq.w	#1,len
		getbit
		beq.s	Fetch10
Back10:
		bcc		Copy

		move.b	(input)+,len
		beq.s	OverNout
		addq.w	#8,len
		bra		Copy

BigDisp:
		getbit
		beq.s	Fetch11
Back11:
		addx.w	pos,pos
		or.w	#4,pos
		getbit
		beq.s	Fetch12
Back12:
		bcs		ByteDisp
		bra		Another

Fetch8:
		reload
		bra.s	Back8
Fetch9:
		reload
		bra.s	Back9
Fetch10:
		reload
		bra.s	Back10
Fetch11:
		reload
		bra.s	Back11
Fetch12:
		reload
		bra.s	Back12
OverNout:
		getbit
		bne.s	Check4end
		reload
Check4end:
		bcs.s	xLoop

		move.w	(%sp)+,%d0
		beq.s	unpack9
#if CHECKSUMS == 1
		move.l	output,%a0
#endif
unpack8:
		move.w	(%sp)+,%d1
#if CHECKSUMS == 1
		move.b	%d1,(%a0)+
#else
		move.b	%d1,(output)+
#endif
		subq.b	#1,%d0
		bne.s	unpack8
unpack9:
#if CHECKSUMS == 1
		move.l	2(%sp),%d0
		sub.l	%d0,output
		move.l	output,%a1
		bsr.s	crc_block
		moveq.l	#UNPACKED_CRC,%d1
		cmp.w	(%sp)+,%d2
		beq.s	unpack11
		ELSEIF
		bra.s	unpack11
#endif
unpack10:
		move.l	%d1,(%sp)
unpack11:
		movem.l	(%sp)+,%d1-%d7/%a2-%a6
		rts

read_long:
		moveq.l	#3,%d1
read_long2:
		lsl.l	#8,%d0
		move.b	(%a0)+,%d0
		dbra	%d1,read_long2
		rts

#if CHECKSUMS == 1
crc_block:
		lea	-0x200(%sp),%sp
		move.l	%sp,%a0
		moveq.l	#0,%d3
crc_block2:
		move.l	%d3,%d1
		moveq.l	#7,%d2
crc_block3:
		lsr.w	#1,%d1
		bcc.s	crc_block4
		eor.w	#CRC_POLY,%d1
crc_block4:
		dbra	%d2,crc_block3
		move.w	%d1,(%a0)+
		addq.b	#1,%d3
		bne.s	crc_block2
		moveq.l	#0,%d2
crc_block5:
		move.b	(%a1)+,%d1
		eor.b	%d1,%d2
		move.w	%d2,%d1
		and.w	#0xff,%d2
		add.w	%d2,%d2
		move.w	(%sp,%d2.w),%d2
		lsr.w	#8,%d1
		eor.b	%d1,%d2
		subq.l	#1,%d0
		bne.s	crc_block5
		lea		0x200(%sp),%sp
		rts
#endif