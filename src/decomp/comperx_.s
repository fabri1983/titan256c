* -----------------------------------------------------------------------------
* Comper-X a newer, much faster implementation of Comper compression
*
* (c) 2021, vladikcomper
* -----------------------------------------------------------------------------
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
* OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
* -----------------------------------------------------------------------------
* INPUT:
*		a0		- Source Offset
*		a1		- Destination Offset
*
* USES:
*		d0-d4, a2-a3
* -----------------------------------------------------------------------------

#include "asm_mac.i"

* -----------------------------------------------------------------------------
* Copy device for RLE transfers
*
* This is located above the compressor for accesibility reasons.
* -----------------------------------------------------------------------------

	rts										// copy length = 0 stops decompression
	*bra.s		.compx_end					// copy length = 0 stops decompression
.rept 127
    move.l		%d4, (%a1)+
.endr
.compx_ComperXDec_CopyDevice_RLE:
    dbf 		%d3, .compx_fetch_flag	    // if bits counter remains, parse the next word
	*moveq		#-1, %d1					// d1 is used for negative sign-extended displacement
	*moveq		#0, %d2						// d2 is used as 8-bit index for copy jump tables
	* ... fall through ... (don't uncomment next instruction)
    *bra.s		.compx_load_flags_field	    

* -----------------------------------------------------------------------------
* Decompressor starts here ...
* -----------------------------------------------------------------------------
* C prototype: extern void ComperXDec (u8* src, u8* dest);
func ComperXDec
	*movem.l     4(%sp), %a0-%a1         	// copy parameters into registers a0-a1
	*movem.l     %a2-%a3/%d2-%d4, -(%sp)    // save registers (except the scratch pad)

	moveq		#-1, %d1				// d1 is used for negative sign-extended displacement
	moveq		#0, %d2					// d2 is used as 8-bit index for copy jump tables

.compx_load_flags_field:
	moveq		#16-1, %d3				// d3 = description field bits counter
	move.w		(%a0)+, %d0				// d0 = description field data

.compx_fetch_flag:
	add.w		%d0, %d0				// roll description field
	bcs.s		.compx_flag			    // if a flag issued, branch
	move.w		(%a0)+, (%a1)+ 			// otherwise, do uncompressed data

.compx_flag_next:
	dbf 		%d3, .compx_fetch_flag		// if bits counter remains, parse the next word
	bra.s		.compx_load_flags_field		// start a new block
* -----------------------------------------------------------------------------
.compx_end:
	*movem.l     (%sp)+, %a2-%a3/%d2-%d4     // restore registers (except the scratch pad)
	rts
* -----------------------------------------------------------------------------
.compx_flag:
    move.b		(%a0)+, %d1				    // d1 = Displacement (words) (sign-extended)
	beq.s		.compx_copy_rle			    // displacement value of 0 (-1) triggers RLE mode
	move.b		(%a0)+, %d2				    // d2 = Copy length field

	add.w		%d1, %d1					// d1 = Displacement * 2 (sign-extended)
	lea 		-2(%a1,%d1.w), %a2			// a2 = Start copy address

	moveq		#-1, %d1					// restore the value of d1 now ...
	add.b		%d2, %d2					// test MSB of copy length field ...
	bcc.s		.compx_copy_long_start	    // if not set, then transfer is even words, branch ...
	move.w		(%a2)+, (%a1)+			    // otherwise, copy odd word before falling into longwords loop ...

.compx_copy_long_start:
	jmp			.compx_ComperXDec_CopyDevice(%pc,%d2.w)	// d2 = 0..$FE
* -----------------------------------------------------------------------------
.compx_copy_rle:
	move.b		(%a0)+, %d1				    // d1 = - $100 + Copy length

	move.w		-(%a1), %d4
	swap		%d4
	move.w		(%a1)+, %d4				    // d4 = data to copy

	add.b		%d1, %d1					// test MSB of copy length field ...
	bcc.s		.compx_copy_long_rle_start	// if not set, then transfer is even words, branch ...
	move.w		%d4, (%a1)+				    // otherwise, copy odd word before falling into longwords loop ...

.compx_copy_long_rle_start:
	* Next instruction not valid using dX register because the destination label is far ahead 128 bytes.
	* jmp			.compx_ComperXDec_CopyDevice_RLE(%pc,%d1.w) // d1 = -$100..-2
	* Workaround: use a jump table
	move.w		%d1, %a3			// d1 = -$100..-2
	jmp			.compx_ComperXDec_CopyDevice_RLE(%a3)
* =============================================================================

* -----------------------------------------------------------------------------
* Copy device for RLE transfers
*
* This is located below the compressor for accesibility reasons.
* -----------------------------------------------------------------------------

.compx_ComperXDec_CopyDevice:
	rts										// copy length = 0 stops decompression
	*bra.s		.compx_end					// copy length = 0 stops decompression
* -----------------------------------------------------------------------------
.rept 127
    move.l		(%a2)+, (%a1)+
.endr
    dbf 		%d3, .compx_fetch_flag		// if bits counter remains, parse the next word
    bra			.compx_load_flags_field
* =============================================================================

* =============================================================================
* -----------------------------------------------------------------------------
* Subroutine to decompress Moduled Comper-X
* -----------------------------------------------------------------------------
* INPUT:
*		a0		- Source Offset
*		a1		- Destination buffer
* -----------------------------------------------------------------------------
* C prototype: extern void ComperXMDec (u8* src, u8* dest);
func ComperXMDec
	*movem.l     4(%sp), %a0-%a1         	// copy parameters into registers a0-a1
	*movem.l     %a2-%a3/%d2-%d4, -(%sp)     // save registers (except the scratch pad)

	lea			ComperXDec(%pc), %a3
	move.w		(%a0)+, %d0
	subq.w		#1, %d0						// this is a trick to reduce number of blocks by one if size is modulo $1000 (4096)
	rol.w		#5, %d0
	and.w		#0x1E, %d0					// d0 = Number of blocks to decompress * 2 (0..1E)
	neg.w		%d0
	jmp			.compxm_decompress_device(%pc,%d0.w)
* -----------------------------------------------------------------------------
.rept 16-1
    jsr			(%a3)
.endr
.compxm_decompress_device:
	jmp			(%a3)
* =============================================================================