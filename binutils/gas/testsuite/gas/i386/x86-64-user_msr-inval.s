# Check Illegal 64bit USER_MSR instructions

	.text
_start:
	urdmsr	$-1, %r14
	urdmsr	$-32767, %r14
	urdmsr	$-2147483648, %r14
	urdmsr	$0x7fffffffffffffff, %r14
	uwrmsr	%r12, $-1
	uwrmsr	%r12, $-32767
	uwrmsr	%r12, $-2147483648
	uwrmsr	%r12, $0x7fffffffffffffff
