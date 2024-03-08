* This file only intended to be used with VSCode plugin: 68k Counter
* Click Toggle counts at the top of this editor windows (scrolling up helps to show the toggle)

* Links:
* See branch instructions here: https://github.com/prb28/m68k-instructions-documentation/blob/master/instructions/bcc.md
* See CCR flags here: https://mrjester.hapisan.com/04_MC68/Sect06Part02/Index.html
* See all instructions here: https://github.com/prb28/m68k-instructions-documentation

* CMP
    cmpi.b  #145,$C00009.l
    cmpi.b  #145,(a0)
    cmpi    #0,d0
    cmp     (a1),d0
    cmpa    d0,a0
    cmp     d1,d0
    cmp     $EEFF0022,d0    ;// cmp memory content to d0
    cmpm    (a0)+,(a1)+     ;// cmp memory to memory (only An registers and both post incremented)

* TST vs CMPI #0
    tst     d0
    * vs
    cmpi    #0,d0

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
    addq.b  #8,$EEFF0022
    addi.w  #12345,$EEFF0022
    addi.w  #12345,d0
    addi.l  #16123456,d0

* Increment a pointer by TITAN_256C_COLORS_PER_STRIP (32) vs just addi instruction
    lea     (64,a0),a0    ;// multiplied by 2 because asm advances in bytes
    move.l  a0,$EEFF0022
    * vs
    addi    #(2*64),$EEFF0022
