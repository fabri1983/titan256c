* -------------------------------------------------------------
* Bytekiller data cruncher v1.1
* Unrolled version
* Improved by Franck "hitchhikr" Charlet.
* fabri1983:
*   - modified C cruncher to add packed length at the beginning of source
*   - modified ASM depacker to extract packed length from source
* -------------------------------------------------------------

#include "asm_mac.i"

.macro GET_BITS
                lsr.l   #1,%d0
                bne.b   0f
                move.l  -(%a0),%d0
                roxr.l  #1,%d0
0:
                addx.l  %d2,%d2
.endm

* -------------------------------------------------------------
* Depacker
* -------------------------------------------------------------
* a0 input
* a1 output
* C prototype: extern void bytekiller_depack (u8* src, u8* dest);
func bytekiller_depack
                * fabri1983: see header notes to know why about next instructions
                move.l  (%a0)+,%d1            // get packed length
                add.l   %d1,%a0               // make %a0 point to the end of the packed data

                moveq   #0,%d2
                move.l  -(%a0),%a2            // retrieve unpacked size
                add.l   %a1,%a2               // end of the unpacked address
                move.l  -(%a0),%d0            // first jmp
bk_not_finished:
                lsr.l   #1,%d0
                bne.b   .bk_not_empty
                move.l  -(%a0),%d0
                roxr.l  #1,%d0
.bk_not_empty:
                bcs.w   bk_big_one
                lsr.l   #1,%d0
                bne.b   .bk_not_empty2
                move.l  -(%a0),%d0
                roxr.l  #1,%d0
.bk_not_empty2:
                bcc.b   bk_do_dupl
                clr.w   %d2
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.b  -1(%a2,%d2.w),-(%a2)
                move.b  -1(%a2,%d2.w),-(%a2)
                cmp.l   %a2,%a1
                blt.b   bk_not_finished
                rts
bk_do_dupl:
                clr.w   %d2
                GET_BITS
                GET_BITS
                GET_BITS
                move.w  %d2,%d3
bk_get_d3_chr2:
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.b  %d2,-(%a2)
                dbf     %d3,bk_get_d3_chr2
                cmp.l   %a2,%a1
                blt.w   bk_not_finished
                rts
bk_big_jmp:
                clr.w   %d2
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.w  %d2,%d3
                addq.w  #8,%d3
bk_get_d3_chr:
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.b  %d2,-(%a2)
                dbf     %d3,bk_get_d3_chr
                cmp.l   %a2,%a1
                blt.w   bk_not_finished
                rts
bk_big_one:
                clr.w   %d2
                GET_BITS
                GET_BITS
                tst.b   %d2
                beq.w   bk_mid_jumps9
                cmp.b   #1,%d2
                beq.w   bk_mid_jumps10
                cmp.b   #3,%d2
                beq.w   bk_big_jmp
                clr.w   %d2
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.w  %d2,%d3
                clr.w   %d2
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
bk_copy_d3_bytes:
                move.b  -1(%a2,%d2.w),-(%a2)
                dbf     %d3,bk_copy_d3_bytes
                cmp.l   %a2,%a1
                blt.w   bk_not_finished
                rts
bk_mid_jumps9:
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.b  -1(%a2,%d2.w),-(%a2)
                move.b  -1(%a2,%d2.w),-(%a2)
                move.b  -1(%a2,%d2.w),-(%a2)
                cmp.l   %a2,%a1
                blt.w   bk_not_finished
                rts
bk_mid_jumps10:
                clr.w   %d2
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                GET_BITS
                move.b  -1(%a2,%d2.w),-(%a2)
                move.b  -1(%a2,%d2.w),-(%a2)
                move.b  -1(%a2,%d2.w),-(%a2)
                move.b  -1(%a2,%d2.w),-(%a2)
                cmp.l   %a2,%a1
                blt.w   bk_not_finished
                rts
