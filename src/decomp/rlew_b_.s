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
;//   rlew_decomp_B_asm
;//
;// DESCRIPTION
;//   Method B: higher compression ratio but slower decompression time.
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
#define RLEW_WIDTH_IN_WORDS_MINUS_1     RLEW_WIDTH_IN_WORDS-1
#define RLEW_ADD_GAP_ON_TARGET          1

;// ||||||||||||||| S U B R O U T I N E |||||||||||||||||||||||||||||||||||||||
;// ---------------------------------------------------------------------------
;// C prototype: extern void rlew_decomp_B_asm (const u8 jumpGap, u8* src, u8* dest);
func rlew_decomp_B_asm
    movem.l     4(sp), d0/a0-a1     ;// copy parameters into registers d0/a0-a1
    movem.l     d2-d7/a2-a4, -(sp)  ;// save registers (except the scratch pad)

    ;// using registers instead of immediate values in some instructions take less cycles
    move.l      d0, a2              ;// a2: jump gap
    moveq       #0, d0              ;// 0 used with cmp and btst instructions
    moveq       #0x40, d4           ;// 0b01000000 parity byte and to test if descriptor is a simple RLE
    move.w      #0x80, d5           ;// 0b10000000 to test if descriptor is an incremental RLE
    move.w      #0xC0, d6           ;// 0b11000000 to test if descriptor is a stream of words
    moveq       #0x3F, d7           ;// 0b00111111 mask for length. NOTE: important to leave higher bytes as 0s
    lea         .b_rlew_get_desc(pc), a3    ;// compared to a bra, jmp (aN) saves 2 cycles
    moveq       #0, d1              ;// clean long word of register before any assignment

    move.b      (a0)+, d1           ;// d1: rows
    subq.b      #1, d1              ;// decrement rows here because we use dbra/dbf for the big loop
    jmp         (a3)                ;// starts decompression

.b_rlew_end_row:
    .if RLEW_ADD_GAP_ON_TARGET
    adda.l      a2, a1              ;// adds the gap used as expanded width in the map
    .endif
    dbra        d1, .b_rlew_get_desc    ;// dbra/dbf: decrement rows, test if rows >= 0 then branch back. When rows = -1 then no branch
    ;// no more rows => quit
    movem.l     (sp)+, d2-d7/a2-a4      ;// restore registers (except the scratch pad)
    rts

;// Operations for a Stream with High Common Byte
    .rept RLEW_WIDTH_IN_WORDS_MINUS_1
    move.w      d3, (a1)+           ;// write the word set in previous step
    move.b      (a0)+, d3           ;// byte goes to lower half of destination leaving the common byte in higher half
    .endr
.b_jmp_stream_cb:
    move.w      d3, (a1)+           ;// write the word set in previous step
    ;// execution just falls to get new descriptor

.b_rlew_get_desc:
    move.b      (a0)+, d2           ;// d2: rleDescriptor
.b_jmp_stream_cb_plus_4b:           ;// this label put here so jump back calculation fits ok
    beq         .b_rlew_end_row     ;// if (descriptor == 0) then is end of row
    cmp.b       d4, d2              ;// test descriptor against 0b01000000 (parity byte)
    bne.s       2f                  ;// not a parity byte? then branch continue with the new segment
    move.b      (a0)+, d2           ;// d2: rleDescriptor
2:
    cmp.b       d4, d2              ;// test rleDescriptor (d2) against 0b01000000 (d4)
    bcs         .b_rlew_rle         ;// if (rleDescriptor < 0b01000000) then is a basic RLE
    *cmp.b       d5, d2              ;// test rleDescriptor (d2) against 0b10000000 (d5)
    *bcs         .b_rlew_inc_rle     ;// if (rleDescriptor < 0b10000000) then is an incremental RLE
    cmp.b       d6, d2              ;// test rleDescriptor (d2) against 0b11000000 (d6)
    bcs         .b_rlew_stream_w    ;// if (rleDescriptor < 0b11000000) then is a stream of words
    ;// it's a stream of a common high byte followed by lower bytes

;// Stream with High Common Byte
.b_rlew_stream_cb:
    and.w       d7, d2              ;// d2: length = rleDescriptor & 0b00111111. Here we know length >= 2
    move.w      (a0)+, d3           ;// d3: higher byte is the common byte, lower byte is a valid one
    ;// prepare jump offset
    add.w       d2, d2
    add.w       d2, d2              ;// d2: length * 4 because every target instruction set takes 4 bytes
    neg.w       d2                  ;// -d2 in 2's Complement so we can jump back
    jmp         .b_jmp_stream_cb_plus_4b(pc,d2.w)

;// Operations for a Stream of Words
    .rept RLEW_WIDTH_IN_WORDS_DIV_2
    move.l	    (a0)+, (a1)+
    .endr
.b_jmp_stream_w:
    jmp         (a3)                ;// jump to get next descriptor

;// Stream of Words
.b_rlew_stream_w:
    and.w       d7, d2              ;// d2: length = rleDescriptor & 0b00111111
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
    jmp         .b_jmp_stream_w(pc,d2.w)

;// Operations for an incremental RLE
    .rept RLEW_WIDTH_IN_WORDS_MINUS_1
    move.w	    d3, (a1)+           ;// write the word set in previous step
    add.w       d7, d3              ;// increment the word
    .endr
.b_jmp_inc_rle:
    move.w	    d3, (a1)+           ;// write the word set in previous step
    moveq       #0x3F, d7           ;// restore d7: 0b00111111 mask for length
.b_jmp_inc_rle_plus_4b:             ;// this label put here so jump back calculation fits ok
    jmp         (a3)                ;// jump to get next descriptor

;// incremental RLE
.b_rlew_inc_rle:
    and.w       d7, d2              ;// d2: length = rleDescriptor & 0b00111111. Here we know length >= 2
    move.b      (a0)+, d7           ;// d7: operator. We will restore it later on.
    move.w      (a0)+, d3           ;// d3: initial value for incremental value copy into output
    ;// prepare jump offset
    add.w       d2, d2
    add.w       d2, d2              ;// d2: length * 4 because every target instruction set takes 4 bytes
    neg.w       d2                  ;// -d2 in 2's Complement so we can jump back
    jmp         .b_jmp_inc_rle_plus_4b(pc,d2.w)

;// Operations for a basic RLE
    .rept RLEW_WIDTH_IN_WORDS_DIV_2
    move.l	    d3, (a1)+
    .endr
.b_jmp_rle:
    jmp         (a3)                ;// jump to get next descriptor

;// basic RLE
.b_rlew_rle:
    move.w      (a0)+, d3           ;// d3: u16 value_w = *(u16*) in
    and.w       d7, d2              ;// d2: length = rleDescriptor & 0b00111111
    btst        d0, d2              ;// test length parity. Here we know length >= 1
    beq.s       2f                  ;// length is even? then branch
    ;// length is odd => copy first word
    move.w      d3, (a1)+           ;// *(u16*) out = value_w
    subq.w      #1, d2              ;// --length
    beq         .b_rlew_get_desc    ;// if length == 0 then jump to get next descriptor
2:
    ;// length is even => copy 2 words (1 long) at a time
    move.w      d3, a4
    swap        d3
    move.w      a4, d3              ;// d3: u32 value_l = (value_w << 16) | value_w;
    ;// prepare jump offset
    neg.w       d2                  ;// -d2 in 2's Complement so we can jump back
                                    ;// every target instruction takes 2 bytes, though d2 comes inherently *2
    jmp         .b_jmp_rle(pc,d2.w)

#undef RLEW_WIDTH_IN_WORDS
#undef RLEW_WIDTH_IN_WORDS_DIV_2
#undef RLEW_WIDTH_IN_WORDS_MINUS_1
#undef RLEW_ADD_GAP_ON_TARGET