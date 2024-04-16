# mach: cr16

	.include "testutils.inc"

	start

	.global read16
read16:
	loadw foo,r1
	cmpw $42, r1
	beq ok
	fail
ok:
	pass

foo:
	.word 42
