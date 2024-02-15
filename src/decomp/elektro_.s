* ELEKTRO UNPACKER
* made by r57shell
* r57shell@uralweb.ru
* elektropage.ru
* Last update: 20.06.2013

* This packer is some sort of LZSS or LZ77 or others.
* This is fast version, but with bigger size.
* Memory needed: 1290 bytes of ROM, and some stack memory
*                to push registers before start.
*                All calculations with registers only.
*                All output memory needs random access.
* Feel free for use.

#include "asm_mac.i"

* some data
idx1:
    dc.l 1
    dc.l 5
    dc.l 0x25
    dc.l 0x125
    dc.l 0x925
    dc.l 0x4925
    dc.l 0x24925
    dc.l 0x124925
    dc.l 0x924925
    dc.l 0x4924925
    dc.l 0x24924925
   
idx2:
    dc.l 1
    dc.l 9
    dc.l 0x49
    dc.l 0x249
    dc.l 0x1249
    dc.l 0x9249
    dc.l 0x49249
    dc.l 0x249249
    dc.l 0x1249249
    dc.l 0x9249249
    dc.l 0x49249249

* Unpacking Function
* a1 input
* a2 output
* returns:
* d0 end of output
* C prototype: extern void elektro_unpack (u8* src, u8* dest);
func elektro_unpack
        * IMPORTANT: given the jump indexes defined above we can't add nor remove any instruction
        movem.l %d2-%d7/%a2-%a3,-(%sp)    // save registers (except the scratch pad)

loc_BBF0:
        move.b  (%a1),%d3
        move.b  %d3,%d1
        lsr.b   #4,%d1
        move.b  %d1,%d0
        lsr.b   #1,%d0
        moveq   #3,%d2
        and.l   %d0,%d2
        clr.b   %d5
        btst    #3,%d1
        bne.w   loc_Bd0C
        tst.b   %d1
        beq.w   loc_BEC8
        lea     (idx1).l,%a3

loc_BC14:
        btst    #0,%d1
        bne.w   loc_BD8E
        move.l  %d5,%d1
        lsl.l   #2,%d1
        andi.l  #0x3FC,%d1
        add.l   (%a3,%d1.l),%d2
        move.l  %d2,%d0
        subq.l  #1,%d0
        bne.w   loc_BCFC

loc_BC32:
        move.b  %d3,%d1
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        move.b  %d1,%d2
        lsr.b   #1,%d2
        moveq   #3,%d4
        and.l   %d2,%d4
        clr.b   %d5
        btst    #3,%d1
        bne.w   loc_BDBE
        tst.b   %d1
        beq.w   loc_BEC8

loc_BC52:
        btst    #0,%d1
        bne.w   loc_BE54
        move.l  %d5,%d3
        lsl.l   #2,%d3
        andi.l  #0x3FC,%d3
        add.l   (%a3,%d3.l),%d4
        move.l  %d4,%d0

loc_BC6A:
        subq.l  #1,%d0
        beq.s   loc_BBF0
        move.b  (%a1),%d3
        andi.b  #0xF0,%d3
        move.b  %d3,(%a2)
        subq.l  #1,%d0
        move.b  (%a1),%d1
        tst.l   %d0
        bne.w   loc_BEB4

loc_BC80:
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        move.b  %d1,%d2
        lsr.b   #1,%d2
        moveq   #3,%d5
        and.l   %d2,%d5
        clr.b   %d6
        btst    #3,%d1
        bne.w   loc_BFC6
        tst.b   %d1
        beq.w   loc_BF40

loc_BC9E:
        btst    #0,%d1
        bne.w   loc_C034
        move.l  %d6,%d1
        lsl.l   #2,%d1
        andi.l  #0x3FC,%d1
        add.l   (%a3,%d1.l),%d5
        move.l  %d5,%d0
        move.l  %d0,%d1
        subq.l  #1,%d1
        bne.w   loc_BE9C

loc_BCBE:
        move.b  (%a1),%d1
        move.b  %d1,%d2
        lsr.b   #4,%d2
        move.b  %d2,%d6
        lsr.b   #1,%d6
        moveq   #3,%d4
        and.l   %d6,%d4
        clr.b   %d6
        btst    #3,%d2
        bne.w   loc_BEE2
        tst.b   %d2
        beq.w   loc_BF40

loc_BCDC:
        btst    #0,%d2
        bne.w   loc_C01C
        move.l  %d6,%d7
        lsl.l   #2,%d7
        andi.l  #0x3FC,%d7
        add.l   (%a3,%d7.l),%d4
        move.l  %d4,%d0
        subq.l  #1,%d0
        beq.s   loc_BC80
        bra.w   loc_BEB4

loc_BCFC:
        addq.l  #1,%a1
        lsl.b   #4,%d3
        move.b  %d3,(%a2)
        move.l  %d0,%d1
        subq.l  #1,%d1
        beq.s   loc_BCBE
        bra.w   loc_BE9C

loc_Bd0C:
        lea     (idx1).l,%a3

loc_Bd12:
        btst    #0,%d1
        bne.w   loc_BDA8
        move.l  %d5,%d7
        lsl.l   #2,%d7
        andi.l  #0x3FC,%d7
        move.l  %d2,%d6
        add.l   (%a3,%d7.l),%d6
        move.b  %d3,%d1
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        move.b  %d1,%d4
        lsr.b   #1,%d4
        moveq   #0,%d2
        move.b  %d4,%d2
        clr.b   %d4
        btst    #0,%d1
        bne.w   loc_BE3E

loc_Bd44:
        move.l  %d4,%d5
        lsl.l   #2,%d5
        andi.l  #0x3FC,%d5
        lea     (idx2).l,%a0
        add.l   (%a0,%d5.l),%d2
        move.l  %d2,%d0
        btst    #0,%d2
        bne.w   loc_BE6C
        lsr.l   #1,%d0
        movea.l %a2,%a0
        suba.l  %d0,%a0

loc_Bd68:
        tst.l   %d6
        beq.w   loc_BBF0
        move.b  (%a0),%d3
        andi.b  #0xF0,%d3
        move.b  %d3,(%a2)
        move.l  %d6,%d2
        subq.l  #1,%d2
        beq.w   loc_BCBE

loc_Bd7E:
        move.b  (%a0)+,%d7
        andi.b  #0xF,%d7
        or.b    %d7,%d3
        move.b  %d3,(%a2)+
        move.l  %d2,%d6
        subq.l  #1,%d6
        bra.s   loc_Bd68

loc_BD8E:
        move.b  %d3,%d1
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        lsl.l   #3,%d2
        move.b  %d1,%d3
        lsr.b   #1,%d3
        moveq   #7,%d4
        and.l   %d3,%d4
        or.l    %d2,%d4
        addq.b  #1,%d5
        bra.w   loc_BC52

loc_BDA8:
        move.b  %d3,%d1
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        lsl.l   #3,%d2
        move.b  %d1,%d6
        lsr.b   #1,%d6
        moveq   #7,%d4
        and.l   %d6,%d4
        or.l    %d2,%d4
        addq.b  #1,%d5

loc_BDBE:
        btst    #0,%d1
        bne.w   loc_BF9A
        move.l  %d5,%d1
        lsl.l   #2,%d1
        andi.l  #0x3FC,%d1
        move.l  %d4,%d6
        add.l   (%a3,%d1.l),%d6
        move.b  (%a1),%d5
        move.b  %d5,%d1
        lsr.b   #4,%d1
        move.b  %d1,%d4
        lsr.b   #1,%d4
        moveq   #0,%d3
        move.b  %d4,%d3
        clr.b   %d4

loc_BDE6:
        btst    #0,%d1
        bne.w   loc_BF48
        move.l  %d4,%d5
        lsl.l   #2,%d5
        andi.l  #0x3FC,%d5
        lea     (idx2).l,%a0
        add.l   (%a0,%d5.l),%d3
        move.l  %d3,%d0
        btst    #0,%d3
        bne.w   loc_BF6A
        lsr.l   #1,%d0
        movea.l %a2,%a0
        suba.l  %d0,%a0
        tst.l   %d6
        beq.w   loc_BEC2

loc_BE18:
        move.b  (%a0),%d3
        andi.b  #0xF0,%d3
        move.b  %d3,(%a2)
        move.l  %d6,%d2
        subq.l  #1,%d2

loc_BE24:
        tst.l   %d2
        beq.w   loc_BF94
        move.b  (%a0)+,%d7
        andi.b  #0xF,%d7
        or.b    %d7,%d3
        move.b  %d3,(%a2)+
        move.l  %d2,%d6
        subq.l  #1,%d6
        beq.w   loc_BEC2
        bra.s   loc_BE18

loc_BE3E:
        move.b  (%a1),%d5
        move.b  %d5,%d1
        lsr.b   #4,%d1
        lsl.l   #3,%d2
        move.b  %d1,%d0
        lsr.b   #1,%d0
        moveq   #7,%d3
        and.l   %d0,%d3
        or.l    %d2,%d3
        addq.b  #1,%d4
        bra.s   loc_BDE6

loc_BE54:
        move.b  (%a1),%d3
        move.b  %d3,%d1
        lsr.b   #4,%d1
        lsl.l   #3,%d4
        move.b  %d1,%d6
        lsr.b   #1,%d6
        moveq   #7,%d2
        and.l   %d6,%d2
        or.l    %d4,%d2
        addq.b  #1,%d5
        bra.w   loc_BC14

loc_BE6C:
        addq.l  #1,%d0
        lsr.l   #1,%d0
        movea.l %a2,%a0
        suba.l  %d0,%a0
        tst.l   %d6
        beq.w   loc_BBF0

loc_BE7A:
        move.b  (%a0)+,%d3
        lsl.b   #4,%d3
        move.b  %d3,(%a2)
        move.l  %d6,%d2
        subq.l  #1,%d2

loc_BE84:
        tst.l   %d2
        beq.w   loc_BCBE
        move.b  (%a0),%d6
        lsr.b   #4,%d6
        or.b    %d6,%d3
        move.b  %d3,(%a2)+
        move.l  %d2,%d6
        subq.l  #1,%d6
        beq.w   loc_BBF0
        bra.s   loc_BE7A

loc_BE9C:
        move.b  (%a1),%d5
        lsr.b   #4,%d5
        or.b    %d5,%d3
        move.b  %d3,(%a2)+
        move.l  %d1,%d0
        subq.l  #1,%d0
        move.b  (%a1),%d3
        tst.l   %d0
        beq.w   loc_BC32
        bra.w   loc_BCFC

loc_BEB4:
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        or.b    %d1,%d3
        move.b  %d3,(%a2)+
        bra.w   loc_BC6A

loc_BEC2:
        move.b  (%a1),%d3
        bra.w   loc_BC32

loc_BEC8:
        movea.l %a2,%a0
        bra.w   loc_C096

loc_BECE:
        move.b  (%a1),%d1
        move.b  %d1,%d2
        lsr.b   #4,%d2
        lsl.l   #3,%d5
        move.b  %d2,%d7
        lsr.b   #1,%d7
        moveq   #7,%d4
        and.l   %d7,%d4
        or.l    %d5,%d4
        addq.b  #1,%d6

loc_BEE2:
        btst    #0,%d2
        bne.w   loc_BFB2
        move.l  %d6,%d5
        lsl.l   #2,%d5
        andi.l  #0x3FC,%d5
        move.l  %d4,%d2
        add.l   (%a3,%d5.l),%d2
        move.b  %d1,%d4
        andi.b  #0xF,%d4
        addq.l  #1,%a1
        move.b  %d4,%d1
        lsr.b   #1,%d1
        moveq   #0,%d7
        move.b  %d1,%d7
        clr.b   %d6

loc_BF0C:
        btst    #0,%d4
        bne.w   loc_C04C
        move.l  %d6,%d4
        lsl.l   #2,%d4
        andi.l  #0x3FC,%d4
        lea     (idx2).l,%a0
        move.l  %d7,%d1
        add.l   (%a0,%d4.l),%d1
        move.l  %d1,%d0
        subq.l  #1,%d0
        btst    #0,%d0
        bne.w   loc_C07C
        lsr.l   #1,%d0
        movea.l %a2,%a0
        suba.l  %d0,%a0
        bra.w   loc_BE84

loc_BF40:
        lea     1(%a2),%a0
        bra.w   loc_C096

loc_BF48:
        move.b  %d5,%d1
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        lsl.l   #3,%d3
        move.b  %d1,%d0
        lsr.b   #1,%d0
        moveq   #7,%d2
        and.l   %d0,%d2
        or.l    %d3,%d2
        addq.b  #1,%d4
        btst    #0,%d1
        beq.w   loc_Bd44
        bra.w   loc_BE3E

loc_BF6A:
        addq.l  #1,%d0
        lsr.l   #1,%d0
        movea.l %a2,%a0
        suba.l  %d0,%a0

loc_BF72:
        tst.l   %d6
        beq.w   loc_BEC2
        move.b  (%a0)+,%d3
        lsl.b   #4,%d3
        move.b  %d3,(%a2)
        move.l  %d6,%d2
        subq.l  #1,%d2

loc_BF82:
        tst.l   %d2
        beq.s   loc_BF94
        move.b  (%a0),%d1
        lsr.b   #4,%d1
        or.b    %d1,%d3
        move.b  %d3,(%a2)+
        move.l  %d2,%d6
        subq.l  #1,%d6
        bra.s   loc_BF72

loc_BF94:
        move.b  (%a1),%d1
        bra.w   loc_BC80

loc_BF9A:
        move.b  (%a1),%d3
        move.b  %d3,%d1
        lsr.b   #4,%d1
        lsl.l   #3,%d4
        move.b  %d1,%d7
        lsr.b   #1,%d7
        moveq   #7,%d2
        and.l   %d7,%d2
        or.l    %d4,%d2
        addq.b  #1,%d5
        bra.w   loc_Bd12

loc_BFB2:
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        lsl.l   #3,%d4
        move.b  %d1,%d0
        lsr.b   #1,%d0
        moveq   #7,%d5
        and.l   %d0,%d5
        or.l    %d4,%d5
        addq.b  #1,%d6

loc_BFC6:
        btst    #0,%d1
        bne.w   loc_BECE
        move.l  %d6,%d4
        lsl.l   #2,%d4
        andi.l  #0x3FC,%d4
        move.l  %d5,%d2
        add.l   (%a3,%d4.l),%d2
        move.b  (%a1),%d5
        move.b  %d5,%d4
        lsr.b   #4,%d4
        move.b  %d4,%d6
        lsr.b   #1,%d6
        moveq   #0,%d1
        move.b  %d6,%d1
        clr.b   %d6

loc_BFEE:
        btst    #0,%d4
        bne.s   loc_C062
        move.l  %d6,%d5
        lsl.l   #2,%d5
        andi.l  #0x3FC,%d5
        lea     (idx2).l,%a0
        add.l   (%a0,%d5.l),%d1
        move.l  %d1,%d0
        subq.l  #1,%d0
        btst    #0,%d0
        bne.s   loc_C08C
        lsr.l   #1,%d0
        movea.l %a2,%a0
        suba.l  %d0,%a0
        bra.w   loc_BF82

loc_C01C:
        andi.b  #0xF,%d1
        addq.l  #1,%a1
        lsl.l   #3,%d4
        move.b  %d1,%d2
        lsr.b   #1,%d2
        moveq   #7,%d5
        and.l   %d2,%d5
        or.l    %d4,%d5
        addq.b  #1,%d6
        bra.w   loc_BC9E

loc_C034:
        move.b  (%a1),%d1
        move.b  %d1,%d2
        lsr.b   #4,%d2
        lsl.l   #3,%d5
        move.b  %d2,%d0
        lsr.b   #1,%d0
        moveq   #7,%d4
        and.l   %d0,%d4
        or.l    %d5,%d4
        addq.b  #1,%d6
        bra.w   loc_BCDC

loc_C04C:
        move.b  (%a1),%d5
        move.b  %d5,%d4
        lsr.b   #4,%d4
        lsl.l   #3,%d7
        move.b  %d4,%d0
        lsr.b   #1,%d0
        moveq   #7,%d1
        and.l   %d0,%d1
        or.l    %d7,%d1
        addq.b  #1,%d6
        bra.s   loc_BFEE

loc_C062:
        move.b  %d5,%d4
        andi.b  #0xF,%d4
        addq.l  #1,%a1
        lsl.l   #3,%d1
        move.b  %d4,%d0
        lsr.b   #1,%d0
        moveq   #7,%d7
        and.l   %d0,%d7
        or.l    %d1,%d7
        addq.b  #1,%d6
        bra.w   loc_BF0C

loc_C07C:
        lsr.l   #1,%d1
        movea.l %a2,%a0
        suba.l  %d1,%a0
        tst.l   %d2
        beq.w   loc_BCBE
        bra.w   loc_Bd7E

loc_C08C:
        lsr.l   #1,%d1
        movea.l %a2,%a0
        suba.l  %d1,%a0
        bra.w   loc_BE24

loc_C096:
        move.l  %a0,%d0
        movem.l (%sp)+,%d2-%d7/%a2-%a3   // restore registers (except the scratch pad)
        rts