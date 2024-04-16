	comment "subroutine prologue"
	.macro gdbasm_enter
	stdu 1, -32(1)
	mflr 0
	std 0, 48(1)
	.endm

	comment "subroutine epilogue"
	.macro gdbasm_leave
	ld 0, 48(1)
	mtlr 0
	ld 1, 0(1)
	blr
	.endm

	.macro gdbasm_call subr
	bl \subr
	nop
	.endm

	.macro gdbasm_several_nops
	nop
	nop
	nop
	nop
	.endm

	comment "exit (0)"
	.macro gdbasm_exit0
	comment "sys_exit"
	li	0, 1
	sc
	.endm

	comment "crt0 startup"
	.macro gdbasm_startup
	.abiversion 2
	.p2align 2
	.endm

	comment "Declare a data variable"
	.purgem gdbasm_datavar
	.macro gdbasm_datavar name value
	.section	".data"
\name:
	.long \value
	.endm
