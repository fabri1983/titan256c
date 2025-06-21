;// ------------------------------------------
;// PackFire 1.2k - (tiny depacker)
;// Minor optimizations made by fabri1983:
;//   - Replaced some bsr calls by macros. This results in more code than original implementation.
;// ------------------------------------------

#include "asm_mac.i"

.macro GET_BIT
                        add.b   d7,d7
                        bne.b   1f
                        move.b  (a2)+,d7
                        addx.b  d7,d7
1:
.endm

.macro GET_BITS
                        moveq   #0,d3
2:                      subq.b  #1,d1
                        bhs.b   3f
                        add.w   d0,d3
                        bra.b   4f
3:                      GET_BIT
                        addx.l  d3,d3
                        bra.b   2b
4:
.endm

;// ------------------------------------------
;// packed data in a0
;// dest in a1
;// -------------------------------------------------------------------------------
;// C prototype: extern void depacker_tiny (u8* in, u8* out);
func depacker_tiny
                        movem.l	4(sp),a0-a1          ;// copy parameters into registers a0-a1
                        movem.l	d2-d7/a2-a3,-(sp)     ;// save used registers (except the scratch pad)

t_start:                lea     26(a0),a2
                        move.b  (a2)+,d7
t_lit_copy:             move.b  (a2)+,(a1)+
t_main_loop:            GET_BIT
                        bcs.b   t_lit_copy
                        moveq   #-1,d3
t_get_index:            addq.l  #1,d3
                        GET_BIT
                        bcc.b   t_get_index
                        cmp.w   #0x10,d3
                        beq.w   t_loop_done
                        bsr.b   t_get_pair
t_continue_1:           move.w  d3,d6              ;// save it for the copy
                        cmp.w   #2,d3
                        ble.b   t_out_of_range
                        moveq   #0,d3
t_out_of_range:         moveq   #0,d0
                        move.b  t_table_len(pc,d3.w),d1
                        move.b  t_table_dist(pc,d3.w),d0
                        GET_BITS
                        bsr.b   t_get_pair
t_continue_2:           move.l  a1,a3
                        sub.l   d3,a3
t_copy_bytes:           move.b  (a3)+,(a1)+
                        subq.w  #1,d6
                        bne.b   t_copy_bytes
                        bra.b   t_main_loop
t_table_len:            dc.b    0x04,0x02,0x04
t_table_dist:           dc.b    0x10,0x30,0x20
t_get_pair:             sub.l   a6,a6
                        moveq   #0xf,d2
t_calc_len_dist:        move.w  a6,d0
                        and.w   d2,d0
                        bne.b   t_node
                        moveq   #1,d5
t_node:                 move.w  a6,d4
                        lsr.w   #1,d4
                        move.b  (a0,d4.w),d1
                        moveq   #1,d4
                        and.w   d4,d0
                        beq.b   t_nibble
                        lsr.b   #4,d1
t_nibble:               move.w  d5,d0
                        and.w   d2,d1
                        lsl.l   d1,d4
                        add.l   d4,d5
                        addq.w  #1,a6
                        dbf     d3,t_calc_len_dist
                        GET_BITS
                        rts
t_loop_done:
                        movem.l (sp)+,d2-d7/a2-a3      ;// restore used registers (except the scratch pad)
                        rts