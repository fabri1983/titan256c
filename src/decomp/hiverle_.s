#include "asm_mac.i"

;// ---------------------------------------------------------------------------
;// Decompress HiveRLE archive to RAM
;//
;// input:
;//	a0 = source address
;//	a1 = destination address
;//
;//	uses d0.w, d1.b, a0, a1, a2, a3
;//
;// ---------------------------------------------------------------------------
;// C prototype: extern void HiveDec (u8* in, u8* out);
func HiveDec
		movem.l	4(sp),a0-a1         ;// copy parameters into registers a0-a1
		move.l	a3,-(sp)            ;// save used registers (except the scratch pad)

HiveDec_Start:
		lea     HiveDec_Copy_Table_End(pc),a2
		lea     HiveDec_Repeat_Table_End(pc),a3
		;//moveq	#-1,d0			;// d0 = $FFnn
		move.w	#$FF00,d0

HiveDec_Next:
		move.b	(a0)+,d0			;// get n byte
		add.b	d0,d0				;// multiply by 2, to both properly adjust to 68K opcode sizes...
		bcs.s	HiveDec_Repeat		;// ...and put repeat flag into the carry flag, hence the byte size
		beq.s	HiveDec_End			;// branch if 0 (including repeat flag)

;// HiveDec_Copy:
		neg.b	d0					;// turn n into -n
		jmp     (a2,d0.w)			;// copy n bytes to destination

HiveDec_Repeat:
		move.b	(a0)+,d1			;// get byte to write
		jmp     (a3,d0.w)			;// write byte n times to destination

HiveDec_End:
		move.l  (sp)+,a3            ;// restore used registers (except the scratch pad)
		rts

HiveDec_Copy_Table:
		.rept	127
		move.b	(a0)+,(a1)+			;// copy byte from source to destination
		.endr
HiveDec_Copy_Table_End:
		bra.w	HiveDec_Next

HiveDec_Repeat_Table:
		.rept	128
		move.b	d1,(a1)+			;// write byte to destination
		.endr
HiveDec_Repeat_Table_End:
		bra.w	HiveDec_Next

;// ---------------------------------------------------------------------------
;// Decompress HiveRLE archive to VRAM.
;// It assumes VDP Auto Inc is already 2.
;//
;// input:
;//	a0 = source address
;//	a1 = RAM buffer address (make sure enough space is set aside)
;//	d0.l = destination address already preconverted to VDP instruction. Eg: VDP_WRITE_VRAM_ADDR(1) indicates VRAM tile 1
;//
;//	uses d0.l, d1.b, d2.l, a0, a1, a2, a3, a4
;//
;// ---------------------------------------------------------------------------
;// C prototype: extern void HiveDecVRAM (u32 vram, u8* in, u8* buf);
func HiveDecVRAM
		movem.l	4(sp),d0/a0-a1      ;// copy parameters into registers d0/a0-a1
		movem.l	d2/a2-a4,-(sp)      ;// save used registers (except the scratch pad)

		lea     ($C00000).l,a4      ;// VDP data port
		move.l	d0,4(a4)			;// set VRAM destination (already converted as VDP instruction)
		;//move.w	#$8F02,4(a4)	;//	set VDP increment to 2 bytes (if not already)
		move.l	a1,d2				;// save start address for buffer
		bsr.w	HiveDec_Start		;// decompress to buffer
	
		move.w	a1,d0				;// get end address for buffer
		sub.w	d2,d0				;// d0 = size of data
		lsr.w	#5,d0				;// divide by 32
		subq.w	#1,d0				;// minus 1 for loop count
		movea.l	d2,a1				;// back to start of buffer

HiveDecVRAM_Loop:
		move.l	(a1)+,(a4)			;// copy from buffer to VRAM
		move.l	(a1)+,(a4)
		move.l	(a1)+,(a4)
		move.l	(a1)+,(a4)
		move.l	(a1)+,(a4)
		move.l	(a1)+,(a4)
		move.l	(a1)+,(a4)
		move.l	(a1)+,(a4)
		dbf     d0,HiveDecVRAM_Loop	;// repeat for all data

        movem.l (sp)+,d2/a2-a4      ;// restore used registers (except the scratch pad)
		rts