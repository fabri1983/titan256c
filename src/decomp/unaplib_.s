*  unaplib_68000.s - aPLib decompressor for 68000 - 156 bytes
*
*  in:  a0 = start of compressed data
*       a1 = start of decompression buffer
*  out: d0 = decompressed size
*
*  Copyright (C) 2020 Emmanuel Marty
*  With parts of the code inspired by Franck "hitchhikr" Charlet
*
*  This software is provided 'as-is', without any express or implied
*  warranty.  In no event will the authors be held liable for any damages
*  arising from the use of this software.
*
*  Permission is granted to anyone to use this software for any purpose,
*  including commercial applications, and to alter it and redistribute it
*  freely, subject to the following restrictions:
*
*  1. The origin of this software must not be misrepresented; you must not
*     claim that you wrote the original software. If you use this software
*     in a product, an acknowledgment in the product documentation would be
*     appreciated but is not required.
*  2. Altered source versions must be plainly marked as such, and must not be
*     misrepresented as being the original software.
*  3. This notice may not be removed or altered from any source distribution.

#include "asm_mac.i"

* C prototype: extern u32 unaplib (u8 *src, u8 *dest);
func unaplib
    movem.l     4(%sp), %a0-%a1     // copy parameters into registers a0-a1

apl_decompress:
    movem.l     %a2-%a6/%d2-%d3, -(%sp)   // save used registers (except the scratch pad)

    moveq       #-128, %d1      // initialize empty bit queue
                                // plus bit to roll into carry
    lea         32000.w, %a2    // load 32000 offset constant
    lea         1280.w, %a3     // load 1280 offset constant
    lea         128.w, %a4      // load 128 offset constant
    move.l      %a1, %a5        // save destination pointer

.apl_literal:
    move.b      (%a0)+, (%a1)+  // copy literal byte
.apl_after_lit:
    moveq       #3, %d2             // set LWM flag

.apl_next_token:
    bsr.s       .apl_get_bit        // read 'literal or match' bit
    bcc.s       .apl_literal        // if 0: literal

    bsr.s       .apl_get_bit        // read '8+n bits or other type' bit
    bcs.s       .apl_other_match    // if 11x: other type of match

    bsr.s       .apl_get_gamma2     // 10: read gamma2-coded high offset bits
    sub.l       %d2, %d0            // high offset bits == 2 when LWM == 3 ?
    bcc.s       .apl_no_repmatch    // if not, not a rep-match

    bsr.s       .apl_get_gamma2     // read repmatch length
    bra.s       .apl_got_len        // go copy large match

.apl_no_repmatch:
    lsl.l       #8, %d0         // shift high offset bits into place
    move.b      (%a0)+, %d0     // read low offset byte
    move.l      %d0, %d3        // copy offset into d3

    bsr.s       .apl_get_gamma2     // read match length
    cmp.l       %a2, %d3            // offset >= 32000 ?
    bge.s       .apl_inc_by_2       // if so, increase match len by 2
    cmp.l       %a3, %d3            // offset >= 1280 ?
    bge.s       .apl_inc_by_1       // if so, increase match len by 1
    cmp.l       %a4, %d3            // offset < 128 ?
    bge.s       .apl_got_len        // if so, increase match len by 2
.apl_inc_by_2:
    addq.l      #1, %d0         // increase match len by 1
.apl_inc_by_1:
    addq.l      #1, %d0         // increase match len by 1

.apl_got_len:
    move.l      %a1, %a6        // calculate backreference address
    sub.l       %d3, %a6        // (dest - match offset)
    subq.l      #1, %d0         // dbf will loop until d0 is -1, not 0
.apl_copy_match:
    move.b      (%a6)+, (%a1)+          // copy matched byte
    dbf         %d0, .apl_copy_match    // loop for all matched bytes
    moveq       #2, %d2                 // clear LWM flag
    bra.s       .apl_next_token         // go decode next token

.apl_other_match:
    bsr.s       .apl_get_bit        // read '7+1 match or short literal' bit
    bcs.s       .apl_short_match    // if 111: 4 bit offset for 1-byte copy

    moveq       #1, %d0         // 110: prepare match length
    moveq       #0, %d3         // clear high bits of offset
    move.b      (%a0)+, %d3     // read low bits of offset + length bit
    lsr.b       #1, %d3         // shift offset into place, len into carry
    beq.s       .apl_done       // check for EOD
    addx.b      %d0, %d0        // len = (1 << 1) + carry bit, ie. 2 or 3
    bra.s       .apl_got_len    // go copy match

.apl_short_match:
    moveq       #0, %d0             // clear short offset before reading 4 bits
    bsr.s       .apl_get_dibits     // read a data bit into d0, one into carry
    addx.b      %d0, %d0            // shift second bit into d0
    bsr.s       .apl_get_dibits     // read a data bit into d0, one into carry
    addx.b      %d0, %d0            // shift second bit into d0
    tst.b       %d0                 // check offset value
    beq.s       .apl_write_zero     // if offset is zero, write a 0

    move.l      %a1, %a6        // calculate backreference address
    sub.l       %d0, %a6        // (dest - short offset)
    move.b      (%a6), %d0      // read matched byte
.apl_write_zero:
    move.b      %d0, (%a1)+     // write matched byte or 0
    bra.s       .apl_after_lit  // set LWM flag and go decode next token

.apl_done:
    move.l      %a1, %d0        // pointer to last decompressed byte + 1
    sub.l       %a6, %d0        // minus start of decompression buffer = size
    movem.l     (%sp)+, %a2-%a6/%d2-%d3    // restore used registers (except the scratch pad)
    rts

.apl_get_gamma2:
    moveq       #1, %d0             // init to 1 so it gets shifted to 2 below
.apl_gamma2_loop:
    bsr.s       .apl_get_dibits     // read data bit, shift into d0
                                    // and read continuation bit
    bcs.s       .apl_gamma2_loop    // loop until a 0 continuation bit is read
    rts

.apl_get_dibits:
    bsr.s       .apl_get_bit    // read bit
    addx.l      %d0, %d0        // shift into d0
                                // fall through
.apl_get_bit:
    add.b       %d1, %d1        // shift bit queue, high bit into carry
    bne.s       .apl_got_bit    // queue not empty, bits remain
    move.b      (%a0)+, %d1     // read 8 new bits
    addx.b      %d1, %d1        // shift bit queue, high bit into carry
                                // and shift 1 from carry into bit queue
.apl_got_bit:
    rts