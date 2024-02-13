* ---------------------------------------------------------------------------
* Original version written by vladikcomper, with improvements by Flamewing
* ---------------------------------------------------------------------------
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
* ---------------------------------------------------------------------------
* FUNCTION:
* 	CompDec
*
* DESCRIPTION
* 	Comper Decompressor
*
* INPUT:
* 	a0	Source address
* 	a1	Destination address
* ---------------------------------------------------------------------------

#include "asm_mac.i"

.macro _Comp_RunBitStream_s 
	dbra		%d3, .comp_mainloop	    // if bits counter remains, parse the next word
	bra.s		.comp_newblock	        // start a new block
.endm

.macro _Comp_ReadBit 
	add.w		%d0, %d0        		// roll description field
.endm

#define _Comp_LoopUnroll 3

* ||||||||||||||| S U B R O U T I N E |||||||||||||||||||||||||||||||||||||||
* ---------------------------------------------------------------------------
* C prototype: extern void ComperDec (u8* src, u8* dest);
func ComperDec
	*movem.l     4(%sp), %a0-%a1         // copy parameters into registers a0-a1
	*movem.l     %a2/%d2-%d5, -(%sp)     // save registers (except the scratch pad)

#if _Comp_LoopUnroll > 0
	moveq		#(1 << _Comp_LoopUnroll)-1, %d5
#endif

.comp_newblock:
	move.w		(%a0)+, %d0		// fetch description field
	moveq		#15, %d3		// set bits counter to 16

.comp_mainloop:
	_Comp_ReadBit
	bcs.s		.comp_flag		// if a flag issued, branch
	move.w		(%a0)+, (%a1)+	// otherwise, do uncompressed data
	_Comp_RunBitStream_s
* ---------------------------------------------------------------------------
.comp_flag:
	moveq		#-1, %d1		    // init displacement
	move.b		(%a0)+, %d1		    // load displacement
	add.w		%d1, %d1
	moveq		#0, %d2			    // init copy count
	move.b		(%a0)+, %d2		    // load copy length
	beq.s		.comp_end			// if zero, branch
	lea	    	(%a1,%d1.w), %a2	// load start copy address
#if _Comp_LoopUnroll > 0
    move.w		%d2, %d4
    not.w		%d4
    and.w		%d5, %d4
    add.w		%d4, %d4
    lsr.w		#_Comp_LoopUnroll, %d2
    jmp	    	.comp_loop(%pc,%d4.w)
#endif
* ---------------------------------------------------------------------------
.comp_loop:
.rept (1 << _Comp_LoopUnroll)
	move.w		(%a2)+, (%a1)+		// copy given sequence
.endr
	dbra		%d2, .comp_loop		// repeat
	_Comp_RunBitStream_s
* ---------------------------------------------------------------------------
.comp_end:
	*movem.l     (%sp)+, %a2/%d2-%d5      // restore registers (except the scratch pad)
	rts
* ===========================================================================