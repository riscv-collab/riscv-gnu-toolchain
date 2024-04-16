# MACRO: exit
	.macro exit nr
	ldd # \nr
	# The debug insn class.
	.byte 0xcd
	# The exit utility function.
	.byte 0x03
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	# Point to the string.
	# NB: See comment above _pass below.
	ldx #0x8000
	# Number of bytes to write.
	ldd #5
	# The debug insn class.
	.byte 0xcd
	# The write utility function.
	.byte 0x01
	exit 0
	.endm

# MACRO: ffail
# Write 'ffail' to stdout and quit
# Normally this would be 'fail', but m68k has a pseudo "fail" op.
	.macro ffail
	# Point to the string.
	ldx #0x8006
	# Number of bytes to write.
	ldd #5
	# The debug insn class.
	.byte 0xcd
	# The write utility function.
	.byte 0x01
	exit 0
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text
# These need to be at the start of text as it's the only stable address.
# The sim will load all sections starting at the .text address and ignore
# the addresses for the other sections.
_pass:
	.asciz "pass\n"
_fail:
	.asciz "fail\n"
.global _start
_start:
	.endm
