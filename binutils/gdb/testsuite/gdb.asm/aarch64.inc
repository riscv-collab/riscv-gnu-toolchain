	comment "subroutine prologue"
	.macro gdbasm_enter
	stp x29, x30, [sp,#-32]!
	mov x29, sp
	.endm

	comment "subroutine epilogue"
	.macro gdbasm_leave
	ldp	x29, x30, [sp],#32
	ret
	.endm

	.macro gdbasm_call subr
	bl \subr
	.endm

	.macro gdbasm_several_nops
	nop
	nop
	nop
	nop
	.endm

	comment "exit (0)"
	.macro gdbasm_exit0
	mov x8, #93
	svc 0
	.endm

	comment "crt0 startup"
	.macro gdbasm_startup
	mov x0, sp
	.endm

