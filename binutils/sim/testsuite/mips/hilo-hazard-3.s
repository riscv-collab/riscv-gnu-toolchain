# Test for mf{hi,lo} -> mult/div/mt{hi,lo} with 2 nops inbetween.
#
# mach:		-mips32r6 -mips64r6 all
# as:		-mabi=eabi
# ld:		-N -Ttext=0x80010000
# output:	pass\\n

	.include "hilo-hazard.inc"
	.include "testutils.inc"

	setup

	.set noreorder
	.ent DIAG
DIAG:
	hilo
	pass
	.end DIAG
