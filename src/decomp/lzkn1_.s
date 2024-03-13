* ===============================================================
* NOTICE
* ===============================================================
* The following snippet is the original decompression code by
* Konami, diassembled from the "Contra Hard Corps" ROM.
* 
* The code syntax targets the ASM68K assembler.
* Modify for the assembler of your choice if neccessary.
* ===============================================================

#include "asm_mac.i"

* ---------------------------------------------------------------
* Konami's LZSS variant 1 (LZKN1) decompressor
* ---------------------------------------------------------------
* INPUT:
*       a0      Input buffer (compressed data location)
*       a1      Output buffer (decompressed data location)
*
* USES:
*       d0-d2, a0-a2
* ---------------------------------------------------------------
* C prototype: void u16 Kon1Dec (u8* in, u8* out)
func Kon1Dec
    movem.l 4(%sp), %a0-%a1    // copy parameters into registers a0-a1
	movem.l	%d2/%a2, -(%sp)    // save registers (except the scratch pad)

    move.w  (%a0)+, %d0        // read uncompressed data size   // NOTICE: not used
    bra.s   .kn1_InitDecomp

* ---------------------------------------------------------------
* Alternative (unused) decompressor entry point
* Skips uncompressed data size at the start of the buffer
*
* NOTICE: This size is not used during decompression anyways.
* ---------------------------------------------------------------
* C prototype: void u16 Kon1Dec2 (u8* in, u8* out)
func Kon1Dec2
    movem.l 4(%sp), %a0-%a1   // copy parameters into registers a0-a1
    movem.l	%d2/%a2, -(%sp)   // save registers (except the scratch pad)
    addq.l  #2, %a0           // skip data size in compressed stream
* ---------------------------------------------------------------

.kn1_InitDecomp:
    lea     (.kn1_MainLoop).l, %a2
    moveq   #0, %d7           // ignore the following DBF

.kn1_MainLoop:
    dbf     %d7, .kn1_RunDecoding // if bits in decription field remain, branch
    moveq   #7, %d7           // set repeat count to 8
    move.b  (%a0)+, %d1       // fetch a new decription field from compressed stream

.kn1_RunDecoding:
    lsr.w   #1, %d1           // shift a bit from the description bitfield
    bcs.w   .kn1_DecodeFlag   // if bit=1, treat current byte as decompression flag
    move.b  (%a0)+, (%a1)+    // if bit=0, treat current byte as raw data
    jmp     (%a2)             // back to @MainLoop
* ---------------------------------------------------------------

.kn1_DecodeFlag:
    moveq   #0, %d0
    move.b  (%a0)+, %d0       // read flag from a compressed stream
    bmi.w   .kn1_Mode10or11   // if bit 7 is set, branch
    cmpi.b  #0x1F, %d0
    beq.w   .kn1_QuitDecomp   // if flag is $1F, branch
    move.l  %d0, %d2          // d2 = %00000000 0ddnnnnn
    lsl.w   #3, %d0           // d0 = %000000dd nnnnn000
    move.b  (%a0)+, %d0       // d0 = Displacement (0..1023)
    andi.w  #0x1F, %d2        // d2 = %00000000 000nnnnn
    addq.w  #2, %d2           // d2 = Repeat count (2..33)
    jmp     (.kn1_UncCopyMode).l
* ---------------------------------------------------------------

.kn1_Mode10or11:
    btst    #6, %d0
    bne.w   .kn1_CompCopyMode // if bits 7 and 6 are set, branch
    move.l  %d0, %d2          // d2 = %00000000 10nndddd
    lsr.w   #4, %d2           // d2 = %00000000 000010nn
    subq.w  #7, %d2           // d2 = Repeat count (1..4)
    andi.w  #0xF, %d0         // d0 = Displacement (0..15)

.kn1_UncCopyMode:
    neg.w   %d0               // negate displacement

.kn1_UncCopyLoop:
    move.b  (%a1,%d0.w), (%a1)+    // self-copy block of uncompressed stream
    dbf     %d2, .kn1_UncCopyLoop  // repeat
    jmp     (%a2)                  // back to @MainLoop
* ---------------------------------------------------------------

.kn1_CompCopyMode:
    subi.b  #0xB9, %d0        // d0 = Repeat count (7..70)

.kn1_CompCopyLoop:
    move.b  (%a0)+, (%a1)+          // copy uncompressed byte
    dbf     %d0, .kn1_CompCopyLoop  // repeat
    jmp     (%a2)                   // back to @MainLoop
* ---------------------------------------------------------------

.kn1_QuitDecomp:
    movem.l	(sp)+, %d2/%a2    // restore registers (except the scratch pad)
    rts