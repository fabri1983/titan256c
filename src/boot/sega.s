#include "task_cst.h"

.section .text.keepboot

*-------------------------------------------------------
*
*       Sega startup code for the GNU Assembler
*       Translated from:
*       Sega startup code for the Sozobon C compiler
*       Written by Paul W. Lee
*       Modified by Charles Coty
*       Modified by Stephane Dallongeville
*
*-------------------------------------------------------

    .globl  rom_header

    .org    0x00000000

_Start_Of_Rom:
_Vecteurs_68K:
        dc.l    __stack                 /* Stack address */
        dc.l    _Entry_Point            /* Program start address */
        dc.l    _Bus_Error
        dc.l    _Address_Error
        dc.l    _Illegal_Instruction
        dc.l    _Zero_Divide
        dc.l    _Chk_Instruction
        dc.l    _Trapv_Instruction
        dc.l    _Privilege_Violation
        dc.l    _Trace
        dc.l    _Line_1010_Emulation
        dc.l    _Line_1111_Emulation
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l    _Error_Exception
        dc.l    _INT
        dc.l    _EXTINT
        dc.l    _INT
        dc.l    hintCaller
        dc.l    _INT
        dc.l    _VINT                   /* _VINT_lean instead of SGDK's _VINT */
        dc.l    _INT
        dc.l    _trap_0                 /* Resume supervisor task */
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT

rom_header:
        .incbin "out/rom_header.bin", 0, 0x100

_Entry_Point:
* disable interrupts
        move    #0x2700,%sr

* Configure a USER_STACK_LENGTH bytes user stack at bottom, and system stack on top of it
        move    %sp, %usp
        sub     #USER_STACK_LENGTH, %sp

* Halt Z80 (need to be done as soon as possible on reset)
        move.l  #0xA11100,%a0       /* Z80_HALT_PORT */
        move.w  #0x0100,%d0
        move.w  %d0,(%a0)           /* HALT Z80 */
        move.w  %d0,0x0100(%a0)     /* END RESET Z80 */

        tst.l   0xa10008
        bne.s   SkipInit

        tst.w   0xa1000c
        bne.s   SkipInit

* Check Version Number
        move.b  -0x10ff(%a0),%d0
        andi.b  #0x0f,%d0
        beq.s   NoTMSS

* Sega Security Code (SEGA)
        move.l  #0x53454741,0x2f00(%a0)

NoTMSS:
        jmp     _start_entry

SkipInit:
        jmp     _reset_entry


*------------------------------------------------
*
*       interrupt functions
*
*------------------------------------------------

_INT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  intCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_EXTINT:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        move.l  eintCB, %a0
        jsr    (%a0)
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

_VINT:
        btst    #5, (%sp)       /* Skip context switch if not in user task */
        bne.s   no_user_task

        tst.w   task_lock
        bne.s   1f
        move.w  #0, -(%sp)      /* TSK_superPend() will return 0 */
        bra.s   unlock          /* If lock == 0, supervisor task is not locked */

1:
        bcs.s   no_user_task    /* If lock < 0, super is locked with infinite wait */
        subq.w  #1, task_lock   /* Locked with wait, subtract 1 to the frame count */
        bne.s   no_user_task    /* And do not unlock if we did not reach 0 */
        move.w  #1, -(%sp)      /* TSK_superPend() will return 1 */

unlock:
        /* Save bg task registers (excepting a7, that is stored in usp) */
        move.l  %a0, task_regs
        lea     (task_regs + UTSK_REGS_LEN), %a0
        movem.l %d0-%d7/%a1-%a6, -(%a0)

        move.w  (%sp)+, %d0     /* Load return value previously pushed to stack */

        move.w  (%sp)+, task_sr /* Pop user task sr and pc, and save them, */
        move.l  (%sp)+, task_pc /* so they can be restored later.          */
        movem.l (%sp)+, %d2-%d7/%a2-%a6 /* Restore non clobberable registers */

no_user_task:
        /* At this point, we always have in the stack the SR and PC of the task */
        /* we want to jump after processing the interrupt, that might be the    */
        /* point where we came from (if there is no context switch) or the      */
        /* supervisor task (if we unlocked it).                                 */

        movem.l %d0-%d1/%a0-%a1,-(%sp)
        ori.w   #0x0001, intTrace           /* in V-Int */
        addq.l  #1, vtimer                  /* increment frame counter (more a vint counter) */

        move.l  z80VIntCB, %a0              /* load Z80 V-Int handler */
        jsr     (%a0)                       /* call user callback */

        btst    #1, VBlankProcess+1         /* PROCESS_BITMAP_TASK ? (use VBlankProcess+1 as btst is a byte operation) */
        beq.s   no_bmp_task

        jsr     BMP_doVBlankProcess         /* do BMP vblank task */

no_bmp_task:
        move.l  vintCB, %a0                 /* load user callback */
        jsr     (%a0)                       /* call user callback */
        andi.w  #0xFFFE, intTrace           /* out V-Int */
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

* Custom version of the _VINT vector which discards User tasks, Bitmap tasks, and XGM tasks, 
* uses usp to backup a0, and immediately calls user's VInt callback.
_VINT_lean:
        movem.l %d0-%d1/%a0, -(%sp)          /* Save the scratch pad since it won't be saved by the function pointed by vintCB */
        move.l  %a1, %usp
        *ori.w   #0x0001, intTrace           /* in V-Int */
        addq.l  #1, vtimer
        move.l  vintCB, %a0
        jsr     (%a0)
        *andi.w  #0xFFFE, intTrace           /* out V-Int */
        move.l  %usp, %a1
        movem.l (%sp)+, %d0-%d1/%a0          /* Restore scratch pad */
        rte
