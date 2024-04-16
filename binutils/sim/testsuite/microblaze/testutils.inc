# MACRO: system_call
# Make a libgloss/Linux system call
	.macro system_call nr:req
	addi r12, r0, \nr;
	brki r14, 8;
	.endm

# MACRO: exit
	.macro exit nr:req
	addi r3, r0, \nr;
	bri 0;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	write 1, 1f, 5
	exit 0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	write 1, 1f, 5
	exit 1
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

# MACRO: write
# Just like the write() C function; uses system calls
	.macro write fd:req, buf:req, count:req
	addi r5, r0, \fd;
	addi r6, r0, \buf;
	addi r7, r0, \count;
	system_call 5
	.endm
