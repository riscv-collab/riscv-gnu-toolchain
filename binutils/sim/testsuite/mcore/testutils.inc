# MACRO: exit
	.macro exit nr
	movi r2, \nr
	# The exit utility function.
	.byte 0x00
	# The debug insn class.
	.byte 0x50
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Trap function 4: write().
	movi r1, 4;
	# Use stdout.
	movi r2, 1;
	# Point to the string.
	lrw r3, 1f;
	# Number of bytes to write.
	movi r4, 5;
	# Trigger OS trap.
	trap 1;
	exit 0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	# Trap function 4: write().
	movi r1, 4;
	# Use stdout.
	movi r2, 1;
	# Point to the string.
	lrw r3, 1f;
	# Number of bytes to write.
	movi r4, 5;
	# Trigger OS trap.
	trap 1;
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
