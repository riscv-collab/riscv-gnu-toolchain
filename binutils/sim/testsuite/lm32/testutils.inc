# MACRO: exit
	.macro exit nr
	mvi r1, \nr;
	# Trap function 1: exit().
	mvi r8, 1;
	scall;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Trap function 5: write().
	mvi r8, 5;
	# Use stdout.
	mvi r1, 1;
	# Point to the string.
	mvhi r2, hi(1f)
	ori r2, r2, lo(1f)
	# Number of bytes to write.
	mvi r3, 5;
	# Trigger OS trap.
	scall;
	exit 0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	# Trap function 5: write().
	mvi r8, 5;
	# Use stdout.
	mvi r1, 1;
	# Point to the string.
	mvhi r2, hi(1f)
	ori r2, r2, lo(1f)
	# Number of bytes to write.
	mvi r3, 5;
	# Trigger OS trap.
	scall;
	exit 0
	.data
	1: .asciz "fail\n"
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.data
.global _fstack
_fstack:
	.rept 0x1024
	.byte 00
	.endr
	.text
.global _start
_start:
	.endm
