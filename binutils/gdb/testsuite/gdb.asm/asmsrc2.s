	.include "common.inc"
	.include "arch.inc"

	comment "Second file in assembly source debugging testcase."

	.global foo2
	gdbasm_declare foo2
	comment "mark: foo2 start"
	gdbasm_enter

	comment "Call someplace else (several times)."

	comment "mark: call foo3"
	gdbasm_call foo3
	gdbasm_call foo3

	comment "All done, return."

	comment "mark: foo2 leave"
	gdbasm_leave
	gdbasm_end foo2
	.section	.note.GNU-stack,"",@progbits
