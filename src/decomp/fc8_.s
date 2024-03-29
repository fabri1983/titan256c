* 
* FC8 compression by Steve Chamberlin
* Derived from liblzg by Marcus Geelnard
* 68000 decompressor by Steve Chamberlin
* 

#include "asm_mac.i"

#define FC8_HEADER_SIZE	8
#define FC8_DECODED_SIZE_OFFSET 4
#define FC8_CHECK_MAGIC_NUMBER 0
#define FC8_CHECK_ENOUGH_OUT_SPACE 0

.macro fc8_movel_incr src, dst
	move.b	(\src)+, (\dst)+
	move.b	(\src)+, (\dst)+
	move.b	(\src)+, (\dst)+
	move.b	(\src)+, (\dst)+
.endm

.macro fc8_movew_incr src, dst
	move.b	(\src)+, (\dst)+
	move.b	(\src)+, (\dst)+
.endm

* lookup table for decoding the copy length-1 parameter
_FC8_LENGTH_DECODE_LUT:
	dc.b	0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11
	dc.b	0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x22,0x2E,0x47,0x7F,0xFF

*-------------------------------------------------------------------------------
* _GetUINT32 - alignment independent reader for 32-bit integers
* a0 = in
* d6 = offset
* d7 = result
* C equivalent: ((u32)in[0]) << 24 | ((u32)in[1]) << 16 | ((u32)in[2]) << 8 | ((u32)in[3])
*-------------------------------------------------------------------------------
#if FC8_CHECK_ENOUGH_OUT_SPACE
_GetUINT32:
	move.b	(%a0,%d6.w), %d7
	asl.w	#8, %d7
	move.b	1(%a0,%d6.w), %d7
	swap	%d7
	move.b	2(%a0,%d6.w), %d7
	asl.w	#8, %d7
	move.b	3(%a0,%d6.w), %d7
	rts
#endif

*-------------------------------------------------------------------------------
* fc8_decode - Decode a compressed memory block
* a0 = in buffer
* a1 = out buffer
* d1 = outsize
* d0 = result (1 if decompression was successful, or 0 upon failure)
*-------------------------------------------------------------------------------
* C prototype: extern u16 fc8_decode_block (u32 outsize, u8* in, u8* out)
func fc8_decode_block
	movem.l	4(%sp), %d1/%a0-%a1			// copy parameters into registers d1/a0-a1
	movem.l	%d2-%d7/%a2-%a6, -(%sp)		// save used registers (except the scratch pad)

_Init_Decode:
#if FC8_CHECK_MAGIC_NUMBER
	* check magic ID
	cmp.b	#'F', (%a0)
	bne		_fail
	cmp.b	#'C', 1(%a0)
	bne		_fail
	cmp.b	#'8', 2(%a0)
	bne		_fail
	cmp.b	#'_', 3(%a0)
	bne		_fail
#endif

#if FC8_CHECK_ENOUGH_OUT_SPACE
	* check decoded size - enough space in the output buffer?
	moveq	#FC8_DECODED_SIZE_OFFSET, %d6
	bsr.s	_GetUINT32
	cmp.l	%d7, %d1
	bhi		_fail
#endif

	* advance a0 to start of compressed data
	lea		FC8_HEADER_SIZE(%a0), %a0

	* a5 = base of length decode lookup table
	*lea		-110(%pc), %a5	// fix PC offset manually after assembly
	lea		_FC8_LENGTH_DECODE_LUT(%pc), %a5

	* helpful constants
	moveq.l	#0x1F.l, %d1
	moveq.l	#0x7.l, %d2
	moveq.l	#0x1.l, %d3
	moveq.l	#0x3F.l, %d4

	* Main decompression loop
_mainloop:
	move.b	(%a0)+, %d6			// d6 = next token
	bmi.s	_BR1_BR2			// BR1 and BR2 tokens have the high bit set

	btst	#6, %d6				// check bit 6 to see if this is a BR0 or LIT token
	bne.s	_BR0

* LIT 00aaaaaa - copy literal string of aaaaaa+1 bytes from input to output	
_LIT:
	move.l	%a0, %a4			// a4 = source ptr for copy
	and.w 	%d4, %d6			// AND with 0x3F, d6 = length-1 word for copy
	lea		1(%a0,%d6.w), %a0	// advance a0 to the end of the literal string
	bra.s 	_copyLoop

* BR0 01baaaaa - copy b+3 bytes from output backref aaaaa to output
_BR0:
	move.b	%d6, %d5
	and.l	%d1, %d5			// AND with 0x1F, d5 = offset for copy = (long)(t0 & 0x1F)
	beq		_done				// BR0 with 0 offset means EOF

	move.l	%a1, %a4
	sub.l	%d5, %a4			// a4 = source ptr for copy

	move.b	(%a4)+, (%a1)+		// copy 3 bytes, can't use move.w. or move.l because src and dest may overlap
	move.b	(%a4)+, (%a1)+
	move.b	(%a4)+, (%a1)+
	btst	#5, %d6				// check b bit
	beq.s	_mainloop
	move.b	(%a4)+, (%a1)+		// copy 1 more byte
	bra.s	_mainloop

_BR1_BR2:
	btst	#6, %d6				// check bit 6 to see if this is a BR1 or BR2 token
	bne.s	_BR2

* BR1 10bbbaaa'aaaaaaaa - copy bbb+3 bytes from output backref aaa'aaaaaaaa to output
_BR1:
	move.b	%d6, %d5
	and.l	%d2, %d5			// AND with 0x07 
	lsl.l	#8, %d5
	move.b	(%a0)+, %d5			// d5 = offset for copy = ((long)(t0 & 0x07) << 8) | t1
	lsr.b	#3, %d6
	and.w	%d2, %d6			// AND with 0x07
	addq.w	#2, %d6				// d6 = length-1 word for copy = ((word)(t0 >> 3) & 0x7) + 1
	bra.s 	_copyBackref

* BR2 11bbbbba'aaaaaaaa'aaaaaaaa - copy lookup_table[bbbbb] bytes from output backref a'aaaaaaaa'aaaaaaaa to output
_BR2:
	move.b	%d6, %d5
	and.w	%d3, %d5			// AND with 0x01
	swap	%d5

	*move.w    (%a0)+, %d5		// d5 = offset for copy = ((long)(t0 & 0x01) << 16) | (t1 << 8) | t2
	move.b	(%a0)+, %d5
	lsl.w	#8, %d5
	move.b	(%a0)+, %d5

	lsr.b	#1, %d6
	and.w	%d1, %d6			// AND with 0x1F
	move.b	(%a5,%d6.w), %d6	// d6 = length-1 word for copy = ((word)(t0 >> 3) & 0x7) + 1
	* fall through to _copyBackref

	* copy data from a previous location in the output buffer
	* d5 = offset from current buffer position	
_copyBackref:
	move.l	%a1, %a4
	sub.l	%d5, %a4			// a4 = source ptr for copy
	cmpi.l 	#4, %d5
	blt.w 	_nearCopy 			// must copy byte-by-byte if offset < 4, to avoid overlapping long copies

	* Partially unrolled block copy. Requires 68020 or better.
	* Uses move.l and move.w where possible, even though both source and dest may be unaligned.
	* It's still faster than multiple move.b instructions
	* d6 = length-1
_copyLoop:
	cmpi.w 	#16, %d6			// test if d6 >= 16
	bge.w 	_copy17orFewer		// if yes then move to _copy17orFewer
	* Next instruction not valid in 68000
	* jmp		_copy16orFewer(%d6.w*2)
	* So here is my workaround:
	move.w	%d6, %d7
	add.w	%d7, %d7		// multiply by 4 because the jump tables use bra.w making the instruction width to be 4
	add.w	%d7, %d7
	jmp		_copy16orFewer(%pc,%d7.w)  // here we can use a byte jump using d7 because destination label is inside 128 bytes

_copy16orFewer:
	bra.w	_copy1
	bra.w	_copy2
	bra.w	_copy3
	bra.w	_copy4
	bra.w	_copy5
	bra.w	_copy6
	bra.w	_copy7
	bra.w	_copy8
	bra.w	_copy9
	bra.w	_copy10
	bra.w	_copy11
	bra.w	_copy12
	bra.w	_copy13
	bra.w	_copy14
	bra.w	_copy15
	bra.w	_copy16

_copy15:
	fc8_movel_incr	%a4, %a1
_copy11:
	fc8_movel_incr	%a4, %a1
_copy7:
	fc8_movel_incr	%a4, %a1
_copy3:
	fc8_movew_incr	%a4, %a1
_copy1:
	move.b 	(%a4)+, (%a1)+
	bra		_mainloop

_copy14:
	fc8_movel_incr	%a4, %a1
_copy10:
	fc8_movel_incr	%a4, %a1
_copy6:
	fc8_movel_incr	%a4, %a1
_copy2:
	fc8_movew_incr	%a4, %a1
	bra		_mainloop

_copy13:
	fc8_movel_incr	%a4, %a1
_copy9:
	fc8_movel_incr	%a4, %a1
_copy5:
	fc8_movel_incr	%a4, %a1
	move.b 	(%a4)+, (%a1)+
	bra		_mainloop

_copy16:
	fc8_movel_incr	%a4, %a1
_copy12:
	fc8_movel_incr	%a4, %a1
_copy8:
	fc8_movel_incr	%a4, %a1
_copy4:
	fc8_movel_incr	%a4, %a1
	bra		_mainloop

_copy17orFewer:
	fc8_movel_incr	%a4, %a1
	fc8_movel_incr	%a4, %a1
	fc8_movel_incr	%a4, %a1
	fc8_movel_incr	%a4, %a1
	subi.w 	#16, %d6		// we just copied 16 bytes
	cmpi.w 	#16, %d6		// test if d6 >= 16
	bge.s 	_copy17orFewer	// if yes then continue with _copy17orFewer. Otherwise copy the remaining bytes < 16
	* Next instruction not valid in 68000
	* jmp		_copy16orFewer(%d6.w*2)
	* So here is my workaround:
	move.w	%d6, %d7
	add.w	%d7, %d7		// multiply by 4 because the jump tables use bra.w making the instruction width to be 4
	add.w	%d7, %d7
	move.w	%d7, %a6
	jmp		_copy16orFewer(%a6)	// we can't use _copy16orFewer(%pc,%d7) because destination label is far than 128 bytes

_nearCopy:
	cmpi.l	#1, %d5
	beq.s	_copyRLE
_nearLoop:
	move.b 	(%a4)+, (%a1)+
	dbf 	%d6, _nearLoop
	bra		_mainloop

_copyRLE:
	* assumes length is at least 3
	move.b	(%a4), (%a1)+		// copy first byte
	btst	#0, %d6
	beq.s	_doRLE				// branch if copy length is odd (because d6 is length-1)
	move.b	(%a4), (%a1)+		// copy second byte
_doRLE:
	subq.w	#2, %d6
	lsr.w	#1, %d6				// length = (length-2) / 2

	*move.w    (%a4), %d5
	move.b	(%a4), %d5
	lsl.w	#8, %d5
	move.b	(%a4), %d5

_rleLoop:
	*move.w    %d5, (%a1)+
	move.w	%d5, %d7
	lsr.w	#8, %d7
	move.b	%d7, (%a1)+
	move.b	%d5, (%a1)+

	dbf		%d6, _rleLoop
	bra		_mainloop

_fail:
	moveq.l	#0, %d0				// result = 0
	bra		_exit

_done:
	moveq.l	#1, %d0				// result = 1

_exit:
	movem.l	(sp)+, %d2-%d7/%a2-%a6	// restore used registers (except the scratch pad)
	rts