# Testcase with unsupported jmp instructions
	.text
	.globl   foo
	.type   foo, @function
foo:
	.cfi_startproc
# The non-zero addend makes control flow tracking not impossible, but
# difficult.  SCFI for such functions is not attempted.
        jmp RangeLimit+1;
RangeLimit:
        nop
        ret
	.cfi_endproc
	.size   foo, .-foo
