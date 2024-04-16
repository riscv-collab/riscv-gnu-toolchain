# MACRO: exit
	.macro exit nr
	ori r5, r0, \nr;
	# Trap function 1: exit().
	ori r4, r0, 1;
	syscall;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Trap function 5: write().
	ori r4, r0, 5;
	# Use stdout.
	ori r5, r0, 1;
	# Point to the string.
	lui r6, %hi(1f);
	ori r6, r6, %lo(1f);
	# Number of bytes to write.
	ori r7, r0, 5;
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
	ori r4, r0, 5;
	# Use stdout.
	ori r5, r0, 1;
	# Point to the string.
	lui r6, %hi(1f);
	ori r6, r6, %lo(1f);
	# Number of bytes to write.
	ori r7, r0, 5;
	# Trigger OS trap.
	syscall;
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
