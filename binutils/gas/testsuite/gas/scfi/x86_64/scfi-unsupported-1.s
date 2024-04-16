# Testcase run with --32 and --x32 (Either not supported with SCFI).
	.text
	.globl   foo
	.type    foo, @function
foo:
	pushq   %rbp
	ret
.LFE0:
	.size   foo, .-foo
