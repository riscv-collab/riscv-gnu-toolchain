# MACRO: outc
# Write byte to stdout
	.macro outc ch
	ldi r16, \ch
	out 0x32, r16
	.endm

# MACRO: exit
	.macro exit nr
	ldi r16, \nr
	out 0x2f, r16
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	outc 'p'
	outc 'a'
	outc 's'
	outc 's'
	outc '\n'
	exit 0
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	outc 'f'
	outc 'a'
	outc 'i'
	outc 'l'
	outc '\n'
	exit 1
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text
.global _start
_start:
	.endm
