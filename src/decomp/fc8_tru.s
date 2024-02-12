#include "compressionTypesTracker.h"
#ifdef USING_FC8

* 
* This version of the FC8 decoder comes from https://github.com/leuat/TRSE
* 
* FC8 compression by Steve Chamberlin
* Derived from liblzg by Marcus Geelnard
* 68000 decompressor by Steve Chamberlin
* 

#include "asm_mac.i"

#define FC8_HEADER_SIZE	8
#define FC8_CHECK_MAGIC_NUMBER 0

*-------------------------------------------------------------------------------
* fc8_decode - Decode a compressed memory block
* a0 = in buffer
* a1 = out buffer
* d0 = result (1 if decompression was successful, or 0 upon failure)
*-------------------------------------------------------------------------------
* C prototype: u16 fc8_decode_block_tru (u8* in, u8* out)
func fc8_decode_block_tru
	movem.l 4(%sp), %a0-%a1		// copy parameters into registers a0-a1
	movem.l	%d2-%d7/%a2-%a6, -(%sp)
	bra 	_Init_Decode

* lookup table for decoding the copy length-1 parameter
_FC8_LENGTH_DECODE_LUT:
	dc.b	0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11
	dc.b	0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x22,0x2E,0x47,0x7F,0xFF

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

	* advance a0 to start of compressed data
	lea		FC8_HEADER_SIZE(%a0), %a0

	* a5 = base of length decode lookup table
	lea		_FC8_LENGTH_DECODE_LUT, %a5

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
	move.l	%a0, %a4				// a4 = source ptr for copy
	and.w 	%d4, %d6				// AND with 0x3F, d6 = length-1 word for copy
	lea		1(%a0,%d6.w), %a0		// advance a0 to the end of the literal string
	bra.s 	_nearLoop

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

_nearLoop:
	move.b 	(%a4)+, (%a1)+
	dbf 	%d6, _nearLoop
	bra		_mainloop

_fail:
	moveq.l	#0, %d0				// result = 0
	bra		_exit

_done:
	moveq.l	#1, %d0				// result = 1

_exit:
	movem.l	(sp)+,%d2-%d7/%a2-%a6
	rts

#endif // USING_FC8