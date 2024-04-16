# mach: cr16

	.include "testutils.inc"

	start

	.global read32
read32:
	loadd foo, (r1,r0)
	cmpd $0x12345678, (r1,r0)
	beq ok
	fail
ok:
	pass

foo:
	.long 0x12345678
