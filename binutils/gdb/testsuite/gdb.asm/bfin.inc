	comment "subroutine prologue"
	.macro gdbasm_enter
	LINK 12;
	.endm

	comment "subroutine epilogue"
	.macro gdbasm_leave
	UNLINK;
	RTS;
	.endm

	.macro gdbasm_call subr
	call \subr;
	.endm

	.macro gdbasm_several_nops
	mnop;
	mnop;
	mnop;
	mnop;
	.endm

	comment "exit (0)"
	.macro gdbasm_exit0
	R0 = 0;
	EXCPT 0;
	NOP;
	.endm

	comment "crt0 startup"
	.macro gdbasm_startup
	FP = 0;
	LINK 0xc;
	.endm

	comment "Declare a data variable"
	.purgem gdbasm_datavar
	.macro gdbasm_datavar name value
	.data
	.align 4
	.type	\name, @object
	.size	\name, 4
\name:
	.long \value
	.endm
