*------------------------------------------------------------------------------
* PRO-PACK Unpack Source Code (Compact Version) - MC68000, Method 2 
*
* Copyright (c) 1991,92 Rob Northen Computing, U.K. All Rights Reserved.
*
* File: RNC_2C.S
*
* Date: 24.03.92
*------------------------------------------------------------------------------

#include "asm_mac.i"

*------------------------------------------------------------------------------
* Equates
*------------------------------------------------------------------------------

#define	input %a0
#define	output %a1
#define	temp %a2

#define	len %d0
#define	pos %d1
#define	bitbuf %d2

*------------------------------------------------------------------------------
* Macros
*------------------------------------------------------------------------------

.macro getbit_rnc2
		add.b	bitbuf,bitbuf
.endm

.macro reload_rnc2
		move.b	(input)+,bitbuf
		addx.b	bitbuf,bitbuf
.endm

.macro getraw_rnc2
		move.b	(input)+,(output)+
.endm

.macro getrawREP_rnc2
7:
		move.b	(input)+,(output)+
		move.b	(input)+,(output)+
		move.b	(input)+,(output)+
		move.b	(input)+,(output)+
		dbra	pos,7b
.endm

*------------------------------------------------------------------------------
* Unpack Routine (Compact Version) - MC68000, Method 2
*
* on entry,
*	a0.l = start address of packed file
*	a1.l = start address to write unpacked file
*	(note: a1 cannot be equal to a0)
*	stack space required: $1C bytes
*
*	all other registers are preserved
*------------------------------------------------------------------------------

* C prototype: extern u32 rnc2_Unpack (u8 *src, u8 *dest);
func rnc2_Unpack
		movem.l 4(%sp), %a0-%a1		// copy parameters into registers a0-a1
		movem.l	%d2/%a2,-(%sp)		// save registers (except the scratch pad)
		lea		18(input),input
		moveq	#-0x80,bitbuf
		add.b	bitbuf,bitbuf
		reload_rnc2
		getbit_rnc2
		bra	rnc2_GetBits2

*------------------------------------------------------------------------------

rnc2_Fetch0:
		reload_rnc2
		bra.s	rnc2_Back0
rnc2_Fetch1:
		reload_rnc2
		bra.s	rnc2_Back1
rnc2_Fetch2:
		reload_rnc2
		bra.s	rnc2_Back2
rnc2_Fetch3:
		reload_rnc2
		bra.s	rnc2_Back3
rnc2_Fetch4:
		reload_rnc2
		bra.s	rnc2_Back4
rnc2_Fetch5:
		reload_rnc2
		bra.s	rnc2_Back5
rnc2_Fetch6:
		reload_rnc2
		bra.s	rnc2_Back6
rnc2_Fetch7:
		reload_rnc2
		bra.s	rnc2_Back7

rnc2_Raw:
		moveq	#3,len
rnc2_x4Bits:
		add.b	bitbuf,bitbuf
		beq.s	rnc2_Fetch0
rnc2_Back0:
		addx.w	pos,pos
		dbra	len,rnc2_x4Bits
		addq.w	#2,pos
		getrawREP_rnc2
		bra.s	rnc2_GetBits2

*------------------------------------------------------------------------------

rnc2_GetLen:
		getbit_rnc2
		beq.s	rnc2_Fetch1
rnc2_Back1:
		addx.w	len,len
		getbit_rnc2
		beq.s	rnc2_Fetch2
rnc2_Back2:
		bcc.s	rnc2_Copy
		subq.w	#1,len
		getbit_rnc2
		beq.s	rnc2_Fetch3
rnc2_Back3:
		addx.w	len,len

		cmp.b	#9,len
		beq.s	rnc2_Raw

*------------------------------------------------------------------------------

rnc2_Copy:
		getbit_rnc2
		beq.s	rnc2_Fetch4
rnc2_Back4:
		bcc.s	rnc2_ByteDisp2
		getbit_rnc2
		beq.s	rnc2_Fetch5
rnc2_Back5:
		addx.w	pos,pos
		getbit_rnc2
		beq.s	rnc2_Fetch6
rnc2_Back6:
		bcs.s	rnc2_BigDisp
		tst.w	pos
		bne.s	rnc2_ByteDisp
		addq.w	#1,pos
rnc2_Another:
		getbit_rnc2
		beq.s	rnc2_Fetch7
rnc2_Back7:
		addx.w	pos,pos

rnc2_ByteDisp:
		rol.w	#8,pos
rnc2_ByteDisp2:
		move.b	(input)+,pos
		move.l	output,temp
		sub.w	pos,temp
		subq.w	#1,temp
		lsr.w	#1,len
		bcc.s	rnc2_ByteDisp3
		move.b	(temp)+,(output)+
rnc2_ByteDisp3:
		subq.w	#1,len
		tst.w	pos
		bne.s	rnc2_ByteDisp5
		move.b	(temp),pos
rnc2_ByteDisp4:
		move.b	pos,(output)+
		move.b	pos,(output)+
		dbra	len,rnc2_ByteDisp4
		bra.s	rnc2_GetBits2
rnc2_ByteDisp5:
		move.b	(temp)+,(output)+
		move.b	(temp)+,(output)+
		dbra	len,rnc2_ByteDisp5
		bra.s	rnc2_GetBits2

*------------------------------------------------------------------------------

rnc2_GetBits:
		reload_rnc2
		bcs.s	rnc2_String
rnc2_xByte:
		getraw_rnc2
rnc2_GetBits2:
		getbit_rnc2
		bcs.s	rnc2_Chkz
		getraw_rnc2
		getbit_rnc2
		bcc.s	rnc2_xByte
rnc2_Chkz:
		beq.s	rnc2_GetBits

*------------------------------------------------------------------------------

rnc2_String:
		moveq	#2,len
		moveq	#0,pos
		getbit_rnc2
		beq.s	rnc2_Fetch8
rnc2_Back8:
		bcc	rnc2_GetLen

rnc2_Smalls:
		getbit_rnc2
		beq.s	rnc2_Fetch9
rnc2_Back9:
		bcc.s	rnc2_ByteDisp2
		addq.w	#1,len
		getbit_rnc2
		beq.s	rnc2_Fetch10
rnc2_Back10:
		bcc.s	rnc2_Copy

		move.b	(input)+,len
		beq.s	rnc2_OverNout
		addq.w	#8,len
		bra.s	rnc2_Copy

rnc2_BigDisp:
		getbit_rnc2
		beq.s	rnc2_Fetch11
rnc2_Back11:
		addx.w	pos,pos
		or.w	#4,pos
		getbit_rnc2
		beq.s	rnc2_Fetch12
rnc2_Back12:
		bcs.s	rnc2_ByteDisp
		bra.s	rnc2_Another

rnc2_Fetch8:
		reload_rnc2
		bra.s	rnc2_Back8
rnc2_Fetch9:
		reload_rnc2
		bra.s	rnc2_Back9
rnc2_Fetch10:
		reload_rnc2
		bra.s	rnc2_Back10
rnc2_Fetch11:
		reload_rnc2
		bra.s	rnc2_Back11
rnc2_Fetch12:
		reload_rnc2
		bra.s	rnc2_Back12
rnc2_OverNout:
		getbit_rnc2
		bne.s	rnc2_Check4end
		reload_rnc2
rnc2_Check4end:
		bcs.s	rnc2_GetBits2
		movem.l	(%sp)+,%d2/%a2		// restore registers (except the scratch pad)
		rts