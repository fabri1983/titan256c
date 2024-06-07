*	*-------+---------------------------------------*
*	|Name	| Denibbler				|
*	|Version| 1.02n		Comment: Final release	|
*	+-------+---------------------------------------+
*	|   Decompresses a .nib file to a RAM address   |
*	|						|
*	|      AsmOne 1.02+, set Label :, ;Comment	|
*	|						|
*	|   /_!_\ Don't optimize the jump tables /_!_\	|
*	|						|
*	|    /_!_\ suba must assemble to suba.  /_!_\	|
*	|						|
*	+-----------------------------------------------+
*	| by Henrik Erlandsson © 2013 bitBrain Studios.	|
*	*-----------------------------------------------*

* License: You may copy or distribute this unmodified source to anyone freely.
* You may modify the source and include its executable code in any demoscene 
* product for the Commodore Amiga only. For all other uses, read/get your 
* license at bitbrain.se.

#include "asm_mac.i"

********************  Nibbler Options  ********************

#define DEN_CPU68000 1		// 1=speed gain for 68000 ONLY. +12b code
#define DEN_SIZEOPTI 0		// 1=1.1% slower, -30b code.

********** MACROS **********

*\1=pattlen
.macro DEN_PATTREP pattlen
	sub.l   %d4,%a3
	move.b  (%a3)+,(%a1)+
.if \pattlen > 1
	move.b  (%a3)+,(%a1)+
.endif
.if \pattlen > 2
	move.b  (%a3)+,(%a1)+
.endif
.if \pattlen > 3
	move.b (%a3)+,(%a1)+
.endif
	move.b (%a6)+,%d1
	bpl.b   DEN_Dinsyncl
	jmp     (%a5)
.endm

********** DECRUNCH DA GREEN, SLIMY TOAD **********
* a0: source
* a1: destiny
* C prototype: extern void Denibble (u8* src, u8* dest);
func Denibble
    movem.l     4(%sp),%a0-%a1            // copy parameters into registers a0-a1
	movem.l     %a2-%a6/%d2-%d7,-(%sp)    // save registers (except the scratch pad)

    addq.w  #4,%a0
    move.w  (%a0)+,%d6
    move.l  (%a0)+,%d0
    move.l  %a0,%a2
    lea     256(%a0),%a0
    lea     (%a0,%d0.l),%a6
    moveq   #-0x10,%d5
    moveq   #0xF,%d7
    lea     DEN_jmpMid(%pc),%a4	// $f0*2 in .b

#if DEN_CPU68000 == 1
    cmp.b   #0x90,%d6
    beq.b   DEN_uses90
    move.w  #0x60F0-2,DEN_p4_54-DEN_jmpMid(%a4)	// bra.b *-$10, like.
DEN_uses90:
#endif

    lea     DEN_Dnotcmnsync(%pc),%a5
    bra.b   DEN_init2

DEN_p22ct:
    move.b  (%a0)+,%d2
    move.b  %d2,%d1
    lsr.b   #4,%d1
    or.b    %d1,%d4
    add.w   %d4,%d4
    suba.l  %d4,%a3
DEN_cl1:
    move.b  (%a3)+,(%a1)+
    move.b  (%a3)+,(%a1)+
    move.b  (%a6)+,%d1
    bpl.b   DEN_insyncl2
    jmp     (%a5)

DEN_jmpT0:
    or.b    %d4,%d1
DEN_insyncl2:
    move.b  (%a2,%d1.w),(%a1)+  // decode and push
    move.b  (%a6)+,%d1
    bpl.b   DEN_insyncl2
    jmp     (%a5)
    nop
    nop

DEN_p4_54:
#if DEN_CPU68000 == 1
    move.b  %d4,-(%sp)
    move.w  (%sp)+,%d4
    move.b  (%a6)+,%d4
    lsl.w   #4,%d4
    bra.w   DEN_p4_54j2
    nop
    nop
#else
    cmp.b   %d6,%d1
    blo.b   DEN_jmpT0
    move.b  %d4,-(%sp)
    move.w  (%sp)+,%d4
    move.b  (%a6)+,%d4
    lsl.w   #4,%d4
    bra.w   DEN_p4_54j2
#endif

DEN_p21:
    add.w   %d4,%d4
    suba.l  %d4,%a3
    bra.w   DEN_cpy2
    nop
    nop

DEN_p22j:
    subq.w  #1,%d3
    addq.b  #4,%d0
    lsl.w   #4,%d4
    not.b   %d7
    bmi.b   DEN_p22ct
    and.b   %d7,%d2
    or.b    %d2,%d4
    add.w   %d4,%d4
    suba.l  %d4,%a3
    bra.b   DEN_cl1

DEN_p32j:
    bra.w   DEN_p32

DEN_init2:
    moveq   #0,%d1
    moveq   #0,%d2
    moveq   #-0x80,%d3	// $80 bclr-value+1 & mask, = 0
    moveq   #0,%d4		// necessary!
    bra.w   DENl

DEN_p33:
    move.b  %d4,-(%sp)
    move.w  (%sp)+,%d4
    move.b  (%a6)+,%d4
    add.w   %d4,%d4
    suba.l  %d4,%a3
    bra.w   DEN_cpy3
    nop

DEN_p4_53:
    move.b  %d4,-(%sp)
    move.w  (%sp)+,%d4
    move.b  (%a6)+,%d4
    bclr    %d3,%d4
    suba.l  %d4,%a3
    beq.w   DEN_cpy4
    bra.w   DEN_cpy5	// no gain from dinsyncl3 here.

******** jumptable divide ******************************

DEN_p4_51:
	not.b	%d7
	bpl.b	DEN_unsync1
	move.b	(%a0)+,%d2
	move.b	%d2,%d4
	lsr.b	#4,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.w	DEN_cpy4
	bra.w	DEN_cpy5
DEN_unsync1:
	moveq	#0xF,%d4
	and.b	%d2,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.w	DEN_cpy4
	bra.w	DEN_cpy5

DEN_p6_73:
	move.b	(%a6)+,%d4
	lsl.w	#4,%d4
	not.b	%d7
	bpl.b	DEN_unsync2
	move.b	(%a0)+,%d2
	move.b	%d2,%d1
	lsr.b	#4,%d1
	or.b	%d1,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.w	DEN_cpy6
	bra.w	DEN_cpy7
DEN_unsync2:
	and.b	%d7,%d2
	or.b	%d2,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.w	DEN_cpy6
	bra.w	DEN_cpy7

********** f0..ff jump table ********** d4 must be cleared or =d0 on entry.

DEN_jmpT:

DEN_jmpMid=DEN_jmpT-0xE0-0x7E

	bra.b	DEN_p31
	bra.b	DEN_decao3
	bra.b	DEN_decao3
	bra.b	DEN_decao3
	bra.b	DEN_decao3
	bra.b	DEN_decao3
	bra.b	DEN_decao3
	bra.b	DEN_decao3
	bra.b	DEN_p4_51
	bra.b	DEN_p8_4
	bra.b	DEN_p6_72
	bra.b	DEN_p6_73
	bra.b	DEN_p6_74
	bra.b	DEN_p8_3
	bra.b	DEN_p8_5
DEN_p4_52:
	move.b	(%a6)+,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.w	DEN_cpy4
	bra.w	DEN_cpy5

DEN_p31:
	moveq	#0,%d4
	bra.b	DEN_p32

DEN_p6_74:
	move.b	(%a6)+,%d4
	move.b	%d4,-(%sp)
	move.w	(%sp)+,%d4
	move.b	(%a6)+,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.b	DEN_cpy6
	bra.b	DEN_cpy7

DEN_decao3:
	move.b	%d4,-(%sp)
	move.w	(%sp)+,%d4
	move.b	(%a6)+,%d4

DEN_p32:
	lsl.w	#4,%d4
	not.b	%d7
	bpl.b	DEN_unsync3
	move.b	(%a0)+,%d2
	move.b	%d2,%d1
	lsr.b	#4,%d1
	or.b	%d1,%d4
	add.w	%d4,%d4
#if DEN_SIZEOPTI == 0
	DEN_PATTREP 3
#else
	suba.l	%d4,%a3
	bra.b	DEN_cpy3
#endif
DEN_unsync3:
	and.b	%d7,%d2
	or.b	%d2,%d4
	add.w	%d4,%d4
#if DEN_SIZEOPTI == 0
	DEN_PATTREP 3
#else
	suba.l	%d4,%a3
	bra.b	DEN_cpy3
#endif

DEN_p6_72:
	move.b	(%a6)+,%d4
	bclr	%d3,%d4
	suba.l	%d4,%a3
	beq.b	DEN_cpy6
	bra.b	DEN_cpy7

DEN_p8_4:
	move.b	(%a6)+,%d4
	move.b	(%a6)+,%d1
	moveq	#0xF,%d0
	and.w	%d1,%d0
	lsl.w	#4,%d4
	lsr.b	#4,%d1
	or.b	%d1,%d4
	bra.b	DEN_p8ct4
	
DEN_p8_3:
	move.b	(%a6)+,%d4
	bra.b	DEN_p8ct2

DEN_p8_5:
	move.b	(%a6)+,%d4
	bmi.b	DEN_uncorEOF
	move.b	%d4,-(%sp)
	move.w	(%sp)+,%d4
	move.b	(%a6)+,%d4
DEN_p8ct2:
	not.b	%d7
	bpl.b	DEN_unsync4
	move.b	(%a0)+,%d2
	move.w	%d2,%d0
	lsr.b	#4,%d0
	bra.b	DEN_p8ct4
DEN_unsync4:
	moveq	#0xF,%d0
	and.b	%d2,%d0
DEN_p8ct4:
	add.w	%d4,%d4
	suba.l	%d4,%a3
DEN_p8_cpyl:
	move.b	(%a3)+,(%a1)+
	dbf     %d0,DEN_p8_cpyl

*    ---  finishing loop for PATT  ---

DEN_cpy7:
	move.b	(%a3)+,(%a1)+
DEN_cpy6:
	move.b	(%a3)+,(%a1)+
DEN_cpy5:
	move.b	(%a3)+,(%a1)+
DEN_cpy4:
	move.b	(%a3)+,(%a1)+
DEN_cpy3:
	move.b	(%a3)+,(%a1)+
DEN_cpy2:
	move.b	(%a3)+,(%a1)+
	move.b	(%a3)+,(%a1)+

*    ---  MAIN LOOP ENTRY  ---

DENl:				// bring the beat back, don't believe the hype.
	move.b	(%a6)+,%d1
	bmi.b	DEN_Dnotcmnsync
DEN_Dinsyncl:
	move.b	(%a2,%d1.w),(%a1)+		// decode and push
	move.b	(%a6)+,%d1
	bpl.b	DEN_Dinsyncl

*2   ---  slice me nice  ---		;d1=$0i, $d0=$0j

DEN_Dnotcmnsync:
	cmp.b	%d5,%d1
	bhs.b	DEN_fx
	moveq	#0xF,%d4
	and.w	%d1,%d4
	beq.b	DEN_unc
	move.l	%a1,%a3
	and.w	%d5,%d1
	jmp     (DEN_jmpT0-8*16)-DEN_jmpMid(%a4,%d1.w)

DEN_p4_54j2:
    not.b	%d7
    bpl.b	DEN_unsync5
    move.b	(%a0)+,%d2
    move.b	%d2,%d1
    lsr.b	#4,%d1
    or.b	%d1,%d4
    bclr	%d3,%d4
    suba.l	%d4,%a3
    beq.b	DEN_cpy4
    bra.b	DEN_cpy5
DEN_unsync5:
    and.b	%d7,%d2
    or.b	%d2,%d4
    bclr	%d3,%d4
    suba.l	%d4,%a3
    beq.b	DEN_cpy4
    bra.b	DEN_cpy5
    
DEN_fx:
    move.w	%d1,%d4
    move.l	%a1,%a3
    add.b	%d1,%d1
    jmp     0x7E(%a4,%d1.w)		// jmpT

********** common/uncommon **********

DEN_uncorEOF:
    cmp.b	%d3,%d4
    beq.b	GOOBY_PLS
DEN_Dinsyncl4:
    move.b  (%a2,%d4.w),(%a1)+		// decode and push
#if DEN_SIZEOPTI == 0
    move.b  (%a6)+,%d4
    bpl.b   DEN_Dinsyncl4
    move.b  %d4,%d1
    jmp     (%a5)
#else
    bra.b   DENl
#endif

*    ---  1 uncbyte out  --- $9x..$dx

DEN_unc:
    cmp.b	%d6,%d1
    blo.b	DEN_Dinsyncl
    not.b	%d7
    bpl.b	DEN_unsync6
    move.b	(%a0)+,%d2
    move.w	%d2,%d0
    lsr.b	#4,%d0
    bra.b	DEN_uncct
DEN_unsync6:
    moveq	#0xF,%d0
    and.b	%d2,%d0
DEN_uncct:
    or.b	%d0,%d1
DEN_Dinsyncl5:
    move.b  (%a2,%d1.w),(%a1)+		// decode and push
#if DEN_SIZEOPTI == 0
    move.b  (%a6)+,%d1
    bpl.b   DEN_Dinsyncl5
    jmp     (%a5)
#else
    bra.b   DENl
#endif
    
GOOBY_PLS:
    movem.l     (%sp)+,%a2-%a6/%d2-%d7      // restore registers (except the scratch pad)
    rts    