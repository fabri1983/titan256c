* This file only intended to be used with VSCode plugin: 68k Counter
* Click Toggle counts at the top of this editor windows (scrolling up helps to show the toggle)

* Links:
* See branch instructions here: https://github.com/prb28/m68k-instructions-documentation/blob/master/instructions/bcc.md
* See CCR flags here: https://mrjester.hapisan.com/04_MC68/Sect06Part02/Index.html
* See all instructions here: https://github.com/prb28/m68k-instructions-documentation

* CMP D0,D1
* Relationship    Signed     Unsigned
* -------------------------------------------------------
* D1 <  D0        BLT        BCS (branch on Carry Set) (instead of BLO?)
* D1 <= D0        BLE        BLS
* D1 =  D0        BEQ        BEQ
* D1 <> D0        BNE        BNE
* D1 >  D0        BGT        BHI
* D1 >= D0        BGE        BCC (branch on Carry Clear) (instead of BHS?)

* CMP
    cmp     d1,d0
    cmpa    d0,a0
    cmpi    #0,d0
    cmp     (a1),d0
    cmpm    (a0)+,(a1)+     ;// cmp memory to memory (only An registers and both post incremented)
    cmpi.b  #145,(a0)
    cmp     $EEFF0022,d0    ;// cmp memory content to d0
    cmpi.b  #145,$C00009

* CMPI #0 vs TST
    cmpi    #0,d0
* vs
    tst     d0

* MOVE
    moveq   #127,d0
    move    #$EEFF,d0       ;// load a constant into d0
    move.l  $EEFF0022,d0    ;// move memory content into d0
    move.l  d0,$EEFF0026    ;// move d0 into memory
    move.l  $EEFF0022,$EEFF0026  ;// memory to memory

* MOVE.l between memory locations vs LEA then MOVE.l to An
    move.l  $EEFF0022,$EEFF0026  ;// memory to memory
* vs
    lea     $EEFF0022,a0
    move.l  a0,$EEFF0026

* ADD
    add.w   d0,d1
    addq.b  #8,d0
    addi.w  #12345,d0
    addi.l  #16123456,d0
    addq.b  #8,$EEFF0022
    addi.w  #12345,$EEFF0022

* Increment a pointer by a constant vs just addi/addq instruction
    lea     (64,a0),a0
    move.l  a0,$EEFF0022
* vs
    addi.b  #255,$EEFF0022
* vs
    addq    #8,$EEFF0022
* vs
    move.l  $EEFF0022,d0
    addi.b  #255,d0

* bool setGradColorForText = vcounterManual >= textRampEffectLimitTop && vcounterManual <= textRampEffectLimitBottom;
* d4 will act as the flag setGradColorForText
    * 76-80 cycles
    moveq       #0,d4          ;// d4: setGradColorForText = 0 (FALSE)
    move.b      $EEFF0022,d0   ;// d0: vcounterManual
    cmp.b       $EEFF0024,d0   ;// cmp: vcounterManual - textRampEffectLimitTop
    blo         .color_batch_1_cmd ;// branch if (vcounterManual < textRampEffectLimitTop) (opposite than vcounterManual >= textRampEffectLimitTop)
    cmp.b       $EEFF0028,d0   ;// cmp: vcounterManual - textRampEffectLimitBottom
    bhi         .color_batch_1_cmd ;// branch if (vcounterManual > textRampEffectLimitBottom) (opposite than vcounterManual <= textRampEffectLimitBottom)
    moveq       #1,d4          ;// d4: setGradColorForText = 1 (TRUE)
* vs
    * 44-46 cycles
    move.b      $EEFF0022,d4   ;// vcounterManual
    * translate vcounterManual to base textRampEffectLimitTop:
    sub.b       $EEFF0024,d4   ;// vcounterManual - textRampEffectLimitTop
    * 32 is the amount of scanlines between top and bottom text ramp effect
    cmpi.b      #32,d4         ;// d4 - 32
    scs.b       d4             ;// d4=0xF if d4 >= 0 then d4=0xFF (enable bg color), if d4 < 0 then d4=0x00 (no bg color)
    *bls         .color_batch_1_cmd ;// branch if d4 <= 32 (at this moment d4 is correctly set)
    * next instruction ended up being redundant, logic works ok without it
    *moveq       #0,d4          ;// d4=0 (no bg color)
.color_batch_1_cmd:


* a4 parity test
*    move.w a4,d7
*    andi.b #1,d7
*    beq    _is_even
*_is_even:


;// Stef's LZ4W copying longs:
  ;// 127*20 = 2540 cycles
  .rept  127
    move.l  (a2)+, (a1)+
  .endr
;// vs:
  ;// 15*152 + 12 + 7*20 = 2432 cycles
  .irp k,0,32,64,96,128,160,192,224,256,288,320,352,384,416,448
    movem.l (a2)+, d2-d7/a5-a6      ;// 8 registers
    movem.l d2-d7/a5-a6, \k(a1)
  .endr
    adda    #480, a1
  .rept  7
    move.l  (a2)+, (a1)+
  .endr

;// Stef's LZ4W copying words:
  ;// 255*12 = 3060 cycles
  .rept  255
    move.w  (a2)+, (a1)+
  .endr
;// vs:
  ;// 31*88 + 12 + 7*12 = 2824 cycles
  .irp k,0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256,272,288,304,320,336,352,368,384,400,416,432,448,464,480
    movem.w (a2)+, d2-d7/a5-a6      ;// 8 registers
    movem.w d2-d7/a5-a6, \k(a1)
  .endr
    adda    #496, a1
  .rept  7
    move.w  (a2)+, (a1)+
  .endr

;// Stef's COPY_MATCH macro
  .macro  COPY_MATCH  count
    add.w  d1, d1
    neg.w  d1
    lea  -2(a1,d1.w), a2            ;// a2 = dst - ((match offset + 1) * 2)
  .rept  ((\count)+1)
    move.w  (a2)+, (a1)+
  .endr
    LZ4W_NEXT
  .endm
;// vs:
  .macro  COPY_MATCH  count
    add.w  d1, d1
    neg.w  d1
    lea  -2(a1,d1.w), a2            ;// a2 = dst - ((match offset + 1) * 2)
  .if (\count + 1) == 2
    move.l  (a2)+, (a1)+
  .else
  .if ((\count + 1) % 2) == 0
  .rept ((\count + 1) / 2) 
    move.l  (a2)+, (a1)+
  .endr
  .else
  .rept ((\count + 1) / 2) 
    move.l  (a2)+, (a1)+
  .endr
    move.w  (a2)+, (a1)+
  .endif
  .endif
    LZ4W_NEXT
  .endm
