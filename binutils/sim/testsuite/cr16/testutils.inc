# r0-r5 are used as tmps, consider them call clobbered by these macros.

	.macro START
	.data
failmsg:
	.ascii "fail\n"
passmsg:
	.ascii "pass\n"
	.text
	.global _START
_START:
	.global _start
_start:
	.endm

	.macro exit rc
	movw $\rc,r2
	movw $0x410,r0
	excp 8
	.endm

	.macro pass
	movw $1, r2
	movd $passmsg,(r4,r3)
	movw $5, r5
	movw $0x404, r0
	excp 8
	exit 0
	.endm

	.macro fail
	movw $1, r2
	movd $failmsg,(r4,r3)
	movw $5, r5
	movw $0x404, r0
	excp 8
	exit 1
	.endm

# Other macros know this only clobbers r0.
	.macro test_h_gr reg, val
	movw $\val,r0
	cmpw \reg, r0
	beq  test_gr
	fail
test_gr:
	.endm

	.macro test_h_grp regp, val
	movd $\val,(r1,r0)
	cmpd \regp,(r1,r0)
	beq  test_grp
	fail
test_grp:
	.endm


	.macro mvi_h_condbit val
	movw $0, r0
	movw $\val, r1
	cmpw r0, r1
	.endm

	.macro test_h_condbit val
	.if \val
	br test_c1
	fail
test_c1:
	.else
	br test_c0
	fail
test_c0:
	.endif
	.endm
