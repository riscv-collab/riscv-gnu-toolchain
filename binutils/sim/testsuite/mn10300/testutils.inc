# MACRO: exit
	.macro exit nr
	mov \nr, d1;
	# Trap function 1: exit().
	mov 1, d0;
	syscall;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Trap function 5: write().
	mov 5, d0;
	# Use stdout.
	mov 1, d1;
	# Point to the string.
	mov 1f, a0;
	mov a0, (12, sp);
	# Number of bytes to write.
	mov 5, d3;
	mov d3, (16, sp);
	# Trigger OS trap.
	syscall;
	exit 0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	# Trap function 5: write().
	mov 5, d0;
	# Use stdout.
	mov 1, d1;
	# Point to the string.
	mov 1f, a0;
	mov a0, (12, sp);
	# Number of bytes to write.
	mov 5, d3;
	mov d3, (16, sp);
	# Trigger OS trap.
	syscall;
	exit 0
	.data
	1: .asciz "fail\n"
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.data
.global _stack
_stack:
	.rept 8
	.long 0
	.endr
	.text
.global _start
_start:
	mov _stack, a0;
	mov a0, sp;
	.endm
