* ---------------------------------------------------------------------------
* This version coded by RealMalachi
* https://sonicresearch.org/community/index.php?threads/improved-enigma-compression.6931/
* ---------------------------------------------------------------------------
* Enigma Decompression Algorithm
* For format explanation see http://info.sonicretro.org/Enigma_compression
* this one is optimised from the original, but with the more rom-intensive
* speedups locked behind some flags down below
* ---------------------------------------------------------------------------
* INPUTS:
* d0 = starting art tile (added to each 8x8 before writing to destination)
* a0 = source address
* a1 = destination address
* TRASHES:
* d0,a0,a1
* STACK:
* - saved registers d1-d7/a2-a6 (13x4 bytes)
* - 4 bytes for one bsr (EniDec_GetInlineCopyVal and EniDec_ChkGetNextByte)
* - 2 bytes for word conversion
* ---------------------------------------------------------------------------

#include "asm_mac.i"

* if 1, stay compatible with the original Enigma
#define _Eni_CompatibilityMode 0

* if 1, allows Enigma compressed files to be at an odd numbered address
#define _Eni_EvenAligned 0

* if 1, saves 22 cycles per loop (12 for SubE) at the cost of some rom space
#define _Eni_RemoveJmpTable 1

* if 1, inlines ChkGetNextByte in EniDec_Loop, for a speedup of 34 cycles per loop
* funny how this simple speedup greatly overshadows _Eni_RemoveJmpTable
* that one required infinitely more effort then this. oh well.
#define _Eni_InlineBitStream 1

* enidecpad16:
* - for RemoveJmpTable, routines needs to be aligned in 16($10) byte chunks
*   none of the routines can exceed that boundary, or the code won't work
*   the only exception to this is SubE; the last one
.macro enidecpad16 routine_label
    .set routine_size, . - \routine_label
    .if routine_size > 16
        .error "ADDR ERROR - EXCEED: routine exceeds 16 bytes!"
    .elseif routine_size < 16
        .set pad_size, 16 - routine_size
        .fill pad_size, 1, 0x00
    .endif
.endm
#define PAD_TO_16_BYTES(label) .fill 16-(.-label),1,0x00

* enidec_checktileflags:
* - this was just repetitive
.macro enidec_checktileflags bit,setmode
    add.b       %d1,%d1
    bcc.s       1f                  // if that bit wasn't set, branch
    subq.w      #1,%d6              // get next bit number
    btst        %d6,%d5             // is this tile flag bit set?
.if \setmode == 0
    ori.w       #1<<\bit,%d3
.else
    addi.w      #1<<\bit,%d3
.endif
1:
.endm
* ===========================================================================

* C prototype: extern void EniDec_opt (u16 mapBaseTileIndex, u8* in, u8* out);
func EniDec_opt
    movem.l     4(%sp), %d0/%a0-%a1         // copy parameters into registers d0/a0-a1
#if _Eni_CompatibilityMode == 0
    movem.l	    %d1-%d7/%a2-%a6,-(%sp)      // save registers (except the scratch pad)
#else
    movem.l     %d0-%d7/%a1-%a6,-(%sp)      // save registers (except the scratch pad)
#endif

* compared to my original implementation, this prevents a race condition
* big thanks to DSK for finding this first
    subq.l      #2,%sp          // allocate 2 bytes from stack
    lea         (%sp),%a6       // use those bytes (via a6) for conversions

* set subroutine loop address
* compared to a bra, jmp (aN) saves 2 cycles per-loop
    lea         opt_EniDec_Loop(%pc),%a5

    movea.w     %d0,%a3         // store starting art tile

    move.b      (%a0)+,%d0
    ext.w       %d0
    movea.w     %d0,%a2         // set initial bit amount for inline copy

    move.b      (%a0)+,%d0      // 000PCCHV ; set vram flag permits
    lsl.b       #3,%d0          // PCCHV000 ; shift by 3
    move.w      %d0,%d2         // store in the high word of d2
    swap        %d2
* set increment word
#if _Eni_EvenAligned == 0
    move.w      (%a0)+,%d4
#else
    move.b      (%a0)+,(%a6)+
    move.b      (%a0)+,(%a6)+
    move.w      -(%a6),%d4
#endif
    add.w       %a3,%d4         // add starting art tile
* set static word
#if _Eni_EvenAligned == 0
    move.w      (%a0)+,%d0
#else
    move.b      (%a0)+,(%a6)+
    move.b      (%a0)+,(%a6)+
    move.w      -(%a6),%d0
#endif
    add.w       %a3,%d0         // add starting art tile
    movea.w     %d0,%a4         // store in a4 (moves and adds are faster on dN.w, saves 4 cycles)
* set initial subroutine flag
#if _Eni_EvenAligned == 0
    move.w      (%a0)+,%d5
#else
    move.b      (%a0)+,(%a6)+
    move.b      (%a0)+,(%a6)+
    move.w      -(%a6),%d5
#endif
* set bit counter
    moveq       #16,%d6         // 16 bits = 2 bytes
opt_EniDec_Loop:
    moveq       #7,%d0          // process 7 bits at a time
    move.w      %d6,%d7         // move d6 to d7
    sub.w       %d0,%d7         // subtract by 7 (convenient)
    move.w      %d5,%d1         // copy d5 into d1
    lsr.w       %d7,%d1         // right shift by value in d7

    move.w      %d1,%d2         // move d1 to d2
    andi.w      #0b01110000,%d1 // keep only 3 bits. Lower 4 are for d2, sign bit unused

    cmpi.w      #1<<6,%d1       // is bit 6 set?
    bhs.s       .opt_prcocess7bits  // if it is, branch
    moveq       #6,%d0          // if not, process 6 bits instead of 7
    lsr.w       #1,%d2          // bitfield now becomes TTSSSS instead of TTTSSSS
.opt_prcocess7bits:
#if _Eni_InlineBitStream == 0
    bsr.w       opt_EniDec_ChkGetNextByte   // uses d0, doesn't touch d1 or d2
#else
*opt_EniDec_ChkGetNextByte:
    sub.w       %d0,%d6         // subtract d0 from d6
    cmpi.w      #8,%d6          // has it hit 8 or lower?
    bhi.s       .opt_nonewbyte_a    // if not, branch
    addq.w      #8,%d6          // 8 bits = 1 byte

    asl.w       #8,%d5          // shift up by a byte
    move.b      (%a0)+,%d5      // store next byte in lower register byte
.opt_nonewbyte_a:
#endif

    moveq       #0xF,%d3        // d3 is also used for SubE
    and.w       %d3,%d2         // keep only lower nybble
#if _Eni_RemoveJmpTable == 0
* JmpTable addresses are word-sized.
* Due to its placement in rom, SubE just falls into itself
    lsr.w       #4-1,%d1        // store upper nybble multiplied by 2 (max value = 7)
    jmp         opt_EniDec_JmpTable(%pc,%d1.w)
#else
* all subroutines are offset by 16 bytes. Some of them barely fit, I'm quite proud of that
* SubE exceeds this, but it's the last one so it doesn't matter
    jmp         opt_EniDec_Sub0(%pc,%d1.w)
#endif
* ---------------------------------------------------------------------------
opt_EniDec_Sub0:
2:
    move.w      %d4,(%a1)+      // write to destination
    addq.w      #1,%d4          // increment
    dbra        %d2,2b          // repeat
    jmp         (%a5)           // EniDec_Loop
#if _Eni_RemoveJmpTable == 1
    *enidecpad16 opt_EniDec_Sub0
    PAD_TO_16_BYTES(opt_EniDec_Sub0)
opt_EniDec_Sub2:
2:
    move.w      %d4,(%a1)+      // write to destination
    addq.w      #1,%d4          // increment
    dbra        %d2,2b          // repeat
    jmp         (%a5)           // EniDec_Loop
    *enidecpad16 opt_EniDec_Sub2
    PAD_TO_16_BYTES(opt_EniDec_Sub2)
#endif
* ---------------------------------------------------------------------------
opt_EniDec_Sub4:
2:
    move.w      %a4,(%a1)+      // write to destination
    dbra        %d2,2b          // repeat
    jmp         (%a5)           // EniDec_Loop
#if _Eni_RemoveJmpTable == 1
    *enidecpad16 opt_EniDec_Sub4
    PAD_TO_16_BYTES(opt_EniDec_Sub4)
opt_EniDec_Sub6:
2:
    move.w      %a4,(%a1)+      // write to destination
    dbra        %d2,2b          // repeat
    jmp         (%a5)           // EniDec_Loop
    *enidecpad16 opt_EniDec_Sub6
    PAD_TO_16_BYTES(opt_EniDec_Sub6)
#endif
* ---------------------------------------------------------------------------
opt_EniDec_Sub8:
    bsr.s       opt_EniDec_GetInlineCopyVal
2:
    move.w      %d1,(%a1)+
    dbra        %d2,2b
    jmp         (%a5)           // EniDec_Loop
#if _Eni_RemoveJmpTable == 1
    *enidecpad16 opt_EniDec_Sub8
    PAD_TO_16_BYTES(opt_EniDec_Sub8)
#endif
* ---------------------------------------------------------------------------
opt_EniDec_SubA:
    bsr.s       opt_EniDec_GetInlineCopyVal
2:
    move.w      %d1,(%a1)+
    addq.w      #1,%d1
    dbra        %d2,2b
    jmp         (%a5)           // EniDec_Loop
#if _Eni_RemoveJmpTable == 1
    *enidecpad16 opt_EniDec_SubA
    PAD_TO_16_BYTES(opt_EniDec_SubA)
#endif
* ---------------------------------------------------------------------------
opt_EniDec_SubC:
    bsr.s       opt_EniDec_GetInlineCopyVal
2:
    move.w      %d1,(%a1)+
    subq.w      #1,%d1
    dbra        %d2,2b
    jmp         (%a5)           // EniDec_Loop
#if _Eni_RemoveJmpTable == 1
    *enidecpad16 opt_EniDec_SubC
    PAD_TO_16_BYTES(opt_EniDec_SubC)
#else
* ---------------------------------------------------------------------------
opt_EniDec_JmpTable:
    bra.s       opt_EniDec_Sub0
    bra.s       opt_EniDec_Sub0     // Sub2
    bra.s       opt_EniDec_Sub4
    bra.s       opt_EniDec_Sub4     // Sub6
    bra.s       opt_EniDec_Sub8
    bra.s       opt_EniDec_SubA
    bra.s       opt_EniDec_SubC
    *bra.s       opt_EniDec_SubE     // fall into SubE
#endif
* ---------------------------------------------------------------------------
* EniDec_SubE is truly a special case
opt_EniDec_SubE:
    cmp.w       %d3,%d2         // d3 = $F ; is the loop set to 16?
    beq.s       opt_EniDec_End  // if so, branch (signifies to end
2:
    bsr.s       opt_EniDec_GetInlineCopyVal
    move.w      %d1,(%a1)+
    dbra        %d2,2b
    jmp         (%a5)           // EniDec_Loop
opt_EniDec_End:
    addq.l      #2,%sp          // deallocate those 2 bytes
#if _Eni_CompatibilityMode == 0
    movem.l     (%sp)+,%d1-%d7/%a2-%a6      // restore registers (except the scratch pad)
#else
* this code figures out where a0 should end
    subq.w	    #1,a0
    cmpi.w	    #16,d6          // were we going to start on a completely new byte?
    bne.s	    .opt_got_byte   // if not, branch
    subq.w	    #1,a0
.opt_got_byte:
.if _Eni_EvenAligned == 0       // TODO: thorough testing
* Orion: small optimization, saves 8-10 cycles
    move.w	    a0,d0
    andi.w	    #1,d0
    adda.w	    d0,a0           // ensure we're on an even byte
.endif
    movem.l     (%sp)+,%d0-%d7/%a1-%a6      // restore registers (except the scratch pad)
#endif
    rts
* ===========================================================================

opt_EniDec_GetInlineCopyVal:
    move.w      %a3,%d3         // starting art tile
* original didn't need to use a high word
* this is a 4 cycle loss, though it's usually made up for everywhere else
    move.l      %d2,%d1         // get vram tile flags
    swap        %d1             // (it's in the high word of d2)
    enidec_checktileflags 15,0
    enidec_checktileflags 14,1
    enidec_checktileflags 13,1
    enidec_checktileflags 12,0
    enidec_checktileflags 11,0

    move.w      %d5,%d1
    move.w      %d6,%d7         // get remaining bits
    sub.w       %a2,%d7         // subtract minimum bit number
    bhs.s       .opt_got_enough     // if we're beyond that, branch
    move.w      %d7,%d6
    addi.w      #16,%d6         // 16 bits = 2 bytes
    neg.w       %d7             // calculate bit deficit
    lsl.w       %d7,%d1         // make space for this many bits
    move.b      (%a0),%d5       // get next byte
    rol.b       %d7,%d5         // make the upper X bits the lower X bits
    add.w       %d7,%d7
    and.w       .opt_andvalues-2(%pc,%d7.w),%d5     // only keep X lower bits
    add.w       %d5,%d1         // compensate for the bit deficit
.opt_got_field:
    move.w      %a2,%d0
    add.w       %d0,%d0
    and.w       .opt_andvalues-2(%pc,%d0.w),%d1     // only keep as many bits as required
    add.w       %d3,%d1         // add starting art tile

*    move.b      (a0)+,d5       // 08 ; get current byte, move onto next byte
*    lsl.w       #8,d5          // 22 ; shift up by a byte
*    move.b      (a0)+,d5       // 08 ; store next byte in lower register byte
                                *  38 cycles total
* saves 4 cycles per branch, at the cost of saving and restoring a6, and setting up the register
* those caveats add around 24 cycles, but from my tests, it usually results in a speedup
    move.b      (%a0)+,(%a6)+   // 12 ; temporarily write into the destination
    move.b      (%a0)+,(%a6)+   // 12
    move.w      -(%a6),%d5      // 10 ; move result to d5, set destination back to correct spot
                                *  34 cycles total
    rts
* ---------------------------------------------------------------------------
.opt_andvalues:
    dc.w        1,3,7,0xF
    dc.w        0x1F,0x3F,0x7F,0xFF
    dc.w        0x1FF,0x3FF,0x7FF,0xFFF
    dc.w        0x1FFF,0x3FFF,0x7FFF,0xFFFF
* ---------------------------------------------------------------------------
.opt_got_exact:
    moveq       #16,%d6         // 16 bits = 2 bytes
    bra.s       .opt_got_field
* ---------------------------------------------------------------------------
.opt_got_enough:
    beq.s       .opt_got_exact  // if the exact number of bits are leftover, branch
    lsr.w       %d7,%d1     // remove unneeded bits
    move.w      %a2,%d0
    add.w       %d0,%d0
    and.w       .opt_andvalues-2(%pc,%d0.w),%d1     // only keep as many bits as required
    add.w       %d3,%d1     // add starting art tile
    move.w      %a2,%d0     // store number of bits used up by inline copy
*    bra.s    EniDec_ChkGetNextByte    // move onto next byte
opt_EniDec_ChkGetNextByte:
    sub.w       %d0,%d6     // subtract d0 from d6
    cmpi.w      #8,%d6      // has it hit 8 or lower?
    bhi.s       .opt_nonewbyte_b    // if not, branch
    addq.w      #8,%d6      // 8 bits = 1 byte
* shift lowest byte to highest byte, and load a new value into low byte
    asl.w       #8,%d5      // 22 ; shift up by a byte
    move.b      (%a0)+,%d5  // 08 ; store next byte in lower register byte
                            *  30 cycles total
*    move.b    d5,(a6)+     // 08
*    move.b    (a0)+,(a6)+  // 12
*    move.w    -(a6),d5     // 10
                            *  30 cycles total, sad.
.opt_nonewbyte_b:
    rts
* ---------------------------------------------------------------------------

#undef _Eni_CompatibilityMode
#undef _Eni_EvenAligned
#undef _Eni_RemoveJmpTable
#undef _Eni_InlineBitStream