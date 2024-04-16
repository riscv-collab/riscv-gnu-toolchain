# Check Illegal 32bit USER_MSR instructions

	.text
_start:
	urdmsr	%r12, %r14
	uwrmsr	%r12, %r14
