# MACRO: exit
	.macro exit nr
	ldi.l $r0, \nr;
	# Trap function 1: exit().
	swi 1;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Use stdout.
	ldi.b $r0, 1;
	# Point to the string.
	ldi.l $r1, 1f;
	# Number of bytes to write.
	ldi.s $r2, 5;
	# Trap function 5: write().
	swi 5;
	exit 0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	# Use stdout.
	ldi.b $r0, 1;
	# Point to the string.
	ldi.l $r1, 1f;
	# Number of bytes to write.
	ldi.s $r2, 5;
	# Trap function 5: write().
	swi 5;
	exit 0
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
