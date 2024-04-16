.include "isa.inc"

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	OUT 'p'
	OUT 'a'
	OUT 's'
	OUT 's'
	OUT '\n'
	HALT
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	OUT 'f'
	OUT 'a'
	OUT 'i'
	OUT 'l'
	OUT '\n'
	HALT
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text
.global _start
_start:
	.endm
