
* This file included from other .s files

.macro func _name, _align=2
    .section .text.asm.\_name
    .globl  \_name
    .type   \_name, @function
    .align  \_align
\_name:
.endm
