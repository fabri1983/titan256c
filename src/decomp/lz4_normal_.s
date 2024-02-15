*---------------------------------------------------------
*
*	LZ4 Frame 68k depacker
*	Written by Arnaud Carré ( @leonard_coder )
*	https://github.com/arnaud-carre/lz4-68k
*
*	LZ4 technology by Yann Collet ( https://lz4.github.io/lz4/ )
*	LZ4 frame description: https://github.com/lz4/lz4/blob/master/doc/lz4_Frame_format.md
*
*---------------------------------------------------------

* input: a0.l : LZ4 frame addr ( 16bits aligned )
*		 a1.l : output buffer
* output: none
*
* Depack data produced with next lz4 command line:
* 	lz4 -9 --no-frame-crc <input_file> <output_file>

#include "asm_mac.i"

#define SKIP_HEADER_CHECKS 0

*---------------------------------------------------------
* C prototype: extern void lz4_frame_depack_normal (u8* src, u8* dest);
func lz4_frame_depack_normal
    *movem.l 4(%sp),%a0-%a1          // copy parameters into registers a0-a1
    *movem.l %d2-%d4/%a2-%a4,-(%sp)	// save registers (except the scratch pad)
#if SKIP_HEADER_CHECKS
    lea		4(%a0),%a0              // skip Magic Number (4 bytes). Faster than addq #4,(%a0)
#else
    * C: if (readU32LittleEndian(src) != 0x184D2204)
    cmpi.l	#0x04224D18,(%a0)+	    // LZ4 frame MagicNb.
    bne.s	lz4_frame_error

    * C: if (0x40 != (src[4] & 0xc9))
    move.b	(%a0),%d0
    andi.b	#0b11001001,%d0         // check version, no depacked size, and no DictID
    cmpi.b	#0b01000000,%d0
    bne.s	lz4_frame_error
#endif

    * C: const uint32_t packedSize = readU32LittleEndian(src + 7);
    * read 32bits block size without movep (little endian)
    move.b	4(%a0),%d0
    swap	%d0
    move.b	6(%a0),%d0
    lsl.l	#8,%d0
    move.b	5(%a0),%d0
    swap	%d0
    move.b	3(%a0),%d0
    lea		7(%a0),%a0              // skip LZ4 block header + packed data size

    bra.s	lz4_depack

lz4_frame_error:
    *movem.l	(%sp)+,%d2-%d4/%a2-%a4  // restore registers (except the scratch pad)
    rts

*---------------------------------------------------------
*
*	LZ4 block 68k depacker
*	Written by Arnaud Carré ( @leonard_coder )
*	https://github.com/arnaud-carre/lz4-68k
*
*	LZ4 technology by Yann Collet ( https://lz4.github.io/lz4/ )
*
*---------------------------------------------------------

* Normal version: 180 bytes ( 1.53 times faster than lz4_smallest.asm )
*
* input: a0.l : packed buffer
*		 a1.l : output buffer
*		 d0.l : LZ4 packed block size (in bytes)
*
* output: none
*

lz4_depack:
    lea		0(%a0,%d0.l),%a4	// packed buffer end

    moveq	#0,%d0
    moveq	#0,%d2
    moveq	#0,%d3
    moveq	#15,%d4
    bra.s	.tokenLoop

.lenOffset:
    move.b	(%a0)+,%d1      // read 16bits offset, little endian, unaligned
    move.b	(%a0)+,-(%sp)
    move.w	(%sp)+,%d3
    move.b	%d1,%d3
    movea.l	%a1,%a3
    sub.l	%d3,%a3
    move.w	%d0,%d1
    cmp.b	%d4,%d1
    bne.s	.small

.readLen0:
    move.b	(%a0)+,%d2
    add.l	%d2,%d1
    not.b	%d2
    beq.s	.readLen0

    addq.l	#4,%d1
.copy:
    move.b	(%a3)+,(%a1)+
    subq.l	#1,%d1
    bne.s	.copy
    bra		.tokenLoop
    
.small:
    add.w	%d1,%d1
    neg.w	%d1
    jmp		.copys(%pc,%d1.w)
.rept 15
    move.b	(%a3)+,(%a1)+
.endr
.copys:
    move.b	(%a3)+,(%a1)+
    move.b	(%a3)+,(%a1)+
    move.b	(%a3)+,(%a1)+
    move.b	(%a3)+,(%a1)+
    
.tokenLoop:
    move.b	(%a0)+,%d0
    move.l	%d0,%d1
    lsr.b	#4,%d1
    beq.s	.lenOffset
    and.w	%d4,%d0
    cmp.b	%d4,%d1
    beq.s	.readLen1

.litcopys:
    add.w	%d1,%d1
    neg.w	%d1
    jmp		.copys2(%pc,%d1.w)
.rept 15
    move.b	(%a0)+,(%a1)+
.endr
.copys2:
    cmpa.l	%a0,%a4
    bne		.lenOffset
    rts
                
.readLen1:
    move.b	(%a0)+,%d2
    add.l	%d2,%d1
    not.b	%d2
    beq.s	.readLen1

.litcopy:
    move.b	(%a0)+,(%a1)+
    subq.l	#1,%d1
    bne.s	.litcopy

    * end test is always done just after literals
    cmpa.l	%a0,%a4
    bne		.lenOffset
    
.over:
    rts						// end