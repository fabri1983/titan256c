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
    *// 76-80 cycles
    moveq       #0,d4
    move.b      $EEFF0022,d0
    cmp.b       $EEFF0026,d0
    blo         .color_batch_1_cmd
    cmp.b       $EEFF0030,d0
    bhi         .color_batch_1_cmd
    moveq       #1,d4
* vs
    *// 58-62 cycles
    move.b      $EEFF0022,d4 ;// vcounterManual
    * translate vcounterManual to base textRampEffectLimitTop:
    sub.b       $EEFF0026,d4 ;// vcounterManual - textRampEffectLimitTop
    * 32 is the amount of scanlines between top and bottom text ramp effect
    cmpi.b      #32,d4 ;// d4 - 32
    spl.b       d4 ;// d4=0xF if d4 >= 0 then d4=0xFF (enable bg color), if d4 < 0 then d4=0x00 (no bg color)
    bls         .color_batch_1_cmd ;// branch if d4 <= 32 (at this moment d4 is correctly set)
    moveq       #0,d4 ;// d4=0 (no bg color)

    eori  #32,$EEFF0022
