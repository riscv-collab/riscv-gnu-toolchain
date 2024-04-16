# MACRO: exit
	.macro exit nr
	mov.w \nr, r1;
	# Trap function 1: exit().
	mov.b #1, r0l;
	ste.b r0l, 0x400;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Use stdout.
	mov.w #1, r1;
	# Point to the string.
	mov.w #1f, r2;
	# Number of bytes to write; push onto stack.
	push.w #5;
	# Adjust as the sim expects 3 byte offset. (!?)
	add.w #-3, sp;
	# Trap function 5: write().
	mov.b #5, r0l;
	ste.b r0l, 0x400;
	exit #0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	# Use stdout.
	mov.w #1, r1;
	# Point to the string.
	mov.w #1f, r2;
	# Number of bytes to write; push onto stack.
	push.w #5;
	# Adjust as the sim expects 3 byte offset. (!?)
	add.w #-3, sp;
	# Trap function 5: write().
	mov.b #5, r0l;
	ste.b r0l, 0x400;
	exit #1
	.data
	1: .asciz "fail\n"
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text
.global _start
_start:
	.endm
