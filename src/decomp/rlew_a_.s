;// ---------------------------------------------------------------------------
;// Original version written by fabri1983
;// ---------------------------------------------------------------------------
;// Permission to use, copy, modify, and/or distribute this software for any
;// purpose with or without fee is hereby granted.
;//
;// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
;// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
;// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
;// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
;// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
;// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
;// OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;// ---------------------------------------------------------------------------
;// FUNCTION:
;//   rlew_decomp_A_asm
;//
;// DESCRIPTION
;//   Method A: lower compression ratio but faster decompression time.
;//   Decompress a RLEW compressed stream of N rows and M words each row, and 
;//   applies an offset in the out buffer at the end of each decompressed row 
;//   only if target is a tilemap with extended width.
;//
;// INPUT:
;//   d0  Jump gap in bytes on destination buffer when a row has been decompressed
;//   a0  Source address
;//   a1  Destination address
;// ---------------------------------------------------------------------------

#include "asm_mac.i"

#define RLEW_WIDTH_IN_WORDS             40  // up to 63
#define RLEW_WIDTH_IN_WORDS_DIV_2       RLEW_WIDTH_IN_WORDS/2
#define RLEW_ADD_GAP_ON_TARGET          1

;// ||||||||||||||| S U B R O U T I N E |||||||||||||||||||||||||||||||||||||||
;// ---------------------------------------------------------------------------
;// C prototype: extern void rlew_decomp_A_asm (const u8 jumpGap, u8* src, u8* dest);
func rlew_decomp_A_asm
    movem.l     4(sp), d0/a0-a1     ;// copy parameters into registers d0/a0-a1
    movem.l     d2-d7/a2-a3, -(sp)  ;// save registers (except the scratch pad)

    ;// using registers instead of immediate values in some instructions take less cycles
    move.l      d0, a2              ;// a2: jump gap
    moveq       #0, d0              ;// 0 used with cmp and btst instructions
    move.b      #0x80, d4           ;// 0b10000000 to test end of row bit
    moveq       #0x3F, d5           ;// 0b00111111 mask for length
    moveq       #6, d6              ;// 7th bit to test for basic RLE mark
    moveq       #0, d7              ;// clean long word of register before any assignment
    moveq       #0, d1              ;// clean long word byte of register before any assignment

    move.b      (a0)+, d1           ;// d1: rows
    subq.b      #1, d1              ;// decrement rows here because we use dbra/dbf for the big loop
    jmp         .a_rlew_get_desc_run_1(pc)  ;// starts decompression with first run

;// Operations for a Stream of Words
    .rept RLEW_WIDTH_IN_WORDS_DIV_2
    move.l	    (a0)+, (a1)+
    .endr
.a_jmp_stream_w:
    cmp.b       d4, d7              ;// test rleDescriptor (d7) against 0b10000000 (d4) => end of row bit set
    bcs         .a_rlew_get_desc    ;// if rleDescriptor < 0b10000000 => branch and continue with next segment
    .if RLEW_ADD_GAP_ON_TARGET
    adda.l      a2, a1              ;// adds the gap used as expanded width in the map
    .endif
    dbra        d1, .a_rlew_get_desc    ;// dbra/dbf: decrement rows, test if rows >= 0 then branch back. When rows = -1 then no branch
    ;// no more rows => quit
    movem.l     (sp)+, d2-d7/a2-a3      ;// restore registers (except the scratch pad)
    rts

.a_rlew_get_desc_run_1:
    move.b      (a0)+, d2           ;// d2: rleDescriptor
    btst        d6, d2              ;// test rleDescriptor bit 7th
    beq         .a_rlew_rle         ;// if bit 7th not set => is a basic RLE
    bra.s       .a_rlew_stream_w    ;// bit 7th set => it's a Stream of Words

.a_rlew_get_desc:
    addq.w      #1, a0              ;// skip parity byte
    move.b      (a0)+, d2           ;// d2: rleDescriptor
    btst        d6, d2              ;// test rleDescriptor bit 7th
    beq         .a_rlew_rle         ;// if bit 7th not set => is a basic RLE
    ;// rleDescriptor has bit 7th set => it's a Stream of Words

;// Stream of Words
.a_rlew_stream_w:
    move.b      d2, d7              ;// backup rleDescriptor to later test end of row bit
    and.w       d5, d2              ;// d2: length = rleDescriptor & 0b00111111
    btst        d0, d2              ;// test length parity. Here we know length >= 2
    beq.s       2f                  ;// length is even? then branch
    ;// length is odd => copy first word
    move.w      (a0)+, (a1)+        ;// *(u16*) out = *(u16*) in
    subq.w      #1, d2              ;// --length
2:
    ;// length is even => copy 2 words (1 long) at a time
    ;// prepare jump offset
    neg.w       d2                  ;// -d2 in 2's Complement so we can jump back
                                    ;// every target instruction takes 2 bytes, though d2 comes inherently *2
    jmp         .a_jmp_stream_w(pc,d2.w)

;// Operations for a basic RLE
    .rept RLEW_WIDTH_IN_WORDS_DIV_2
    move.l	    d3, (a1)+
    .endr
.a_jmp_rle:
    cmp.b       d4, d7              ;// test rleDescriptor (d7) against 0b10000000 (d4) => end of row bit set
    bcs         .a_rlew_get_desc    ;// if rleDescriptor < 0b10000000 => branch and continue with next segment
    .if RLEW_ADD_GAP_ON_TARGET
    adda.l      a2, a1              ;// adds the gap used as expanded width in the map
    .endif
    dbra        d1, .a_rlew_get_desc    ;// dbra/dbf: decrement rows, test if rows >= 0 then branch back. When rows = -1 then no branch
    ;// no more rows => quit
    movem.l     (sp)+, d2-d7/a2-a3      ;// restore registers (except the scratch pad)
    rts

;// basic RLE
.a_rlew_rle:
    move.b      d2, d7              ;// backup rleDescriptor to later test end of row bit
    move.w      (a0)+, d3           ;// d3: u16 value_w = *(u16*) in
    and.w       d5, d2              ;// d2: length = rleDescriptor & 0b00111111
    btst        d0, d2              ;// test length parity. Here we know length >= 1
    beq.s       2f                  ;// length is even? then branch
    ;// length is odd => copy first word
    move.w      d3, (a1)+           ;// *(u16*) out = value_w
    subq.w      #1, d2              ;// --length
    beq         .a_rlew_get_desc    ;// if length == 0 then jump to get next descriptor
2:
    ;// length is even => copy 2 words (1 long) at a time
    move.w      d3, a3
    swap        d3
    move.w      a3, d3              ;// d3: u32 value_l = (value_w << 16) | value_w;
    ;// prepare jump offset
    neg.w       d2                  ;// -d2 in 2's Complement so we can jump back
                                    ;// every target instruction takes 2 bytes, though d2 comes inherently *2
    jmp         .a_jmp_rle(pc,d2.w)

#undef RLEW_WIDTH_IN_WORDS
#undef RLEW_WIDTH_IN_WORDS_DIV_2
#undef RLEW_ADD_GAP_ON_TARGET