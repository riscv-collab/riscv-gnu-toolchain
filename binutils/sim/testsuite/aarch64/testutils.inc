# MACRO: exit
# Terminates execution.
	.macro exit nr

	stp	x29, x30, [sp,#-32]!
	mov	x4, #0x26
	mov	x7, #\nr
	mov	x29, sp
	movk	x4, #0x2, lsl #16
	add	x1, x29, #0x10
	str	x4, [x29,#16]
	str	x7, [x29,#24]
	mov	w0, #0x18
	hlt	#0xf000

	.endm

# MACRO: swiwrite
# Writes the string in X1 to stdout
	.macro swiwrite len
	
	stp	x29, x30, [sp,#-48]!
	mov	x0, #1
	mov	x2, #\len
	mov	x29, sp
	str	x0, [x29,#24]
	str	x1, [x29,#32]
	str	x2, [x29,#40]
	mov	w0, #0x5
	add	x1, x29, #0x18
	hlt	#0xf000
	ldp	x29, x30, [sp],#48
	ret

	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass

	adrp	x1, .Lpass
	add	x1, x1, :lo12:.Lpass
	
	swiwrite 5
	exit 0
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail

	adrp	x1, .Lfail
	add	x1, x1, :lo12:.Lfail
	swiwrite 5
	exit 1
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.data
.Lpass:
	.asciz "pass\n"
.Lfail:
	.asciz "fail\n"

	.text
.global _start
_start:
	.endm
