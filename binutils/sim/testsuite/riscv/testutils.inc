# MACRO: exit
	.macro exit nr
	li a0, \nr
	# The exit utility function.
	li a7, 93;
	# Trigger OS trap.
	ecall;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit.
	.macro pass
	# syscall write().
	li a7, 64;
	# Use stdout.
	li a0, 1;
	# Point to the string.
	lla a1, 1f;
	# Number of bytes to write.
	li a2, 5;
	# Trigger OS trap.
	ecall;
	exit 0;
	.pushsection .data
	1: .asciz "pass\n"
	.popsection
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit.
	.macro fail
	# syscall write().
	li a7, 64;
	# Use stdout.
	li a0, 1;
	# Point to the string.
	la a1, 1f;
	# Number of bytes to write.
	li a2, 5;
	# Trigger OS trap.
	ecall;
	exit 0;
	.pushsection .data
	1: .asciz "fail\n"
	.popsection
	.endm

# MACRO: start
# All assembler tests should start with a call to "start".
	.macro start
	.text
.global _start
_start:
	.option push
	.option norelax
	lla gp, __global_pointer$
	.option pop
	.endm
