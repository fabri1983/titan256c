* This file only intended to be used with VSCode plugin: 68k Counter
* Click Toggle counts at the top of this editor windows (scrolling up helps to show the toggle)

* Links:
* See branch instructions here: https://github.com/prb28/m68k-instructions-documentation/blob/master/instructions/bcc.md
* See CCR flags here: https://mrjester.hapisan.com/04_MC68/Sect06Part02/Index.html
* See all instructions here: https://github.com/prb28/m68k-instructions-documentation

* CMP
    cmpi.b  #145,$C00009.l
    cmpi.b  #145,(a0)
    cmpa    d0,a0
    cmp     (a1),d0
    cmp     d1,d0
    cmp     $EEFF0022,d0  // cmp memory content to d0
    cmpm    (a0)+,(a1)+   // cmp memory to memory (only An registers and both post incremented)

* MOVE
    moveq   #127,d0
    move    #$EEFF,d0     // load a constant into d0
    move    $EEFF0022,d0  // move memory content into d0

