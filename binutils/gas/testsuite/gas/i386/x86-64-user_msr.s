# Check 64bit USER_MSR instructions

	.text
_start:
	urdmsr	%r14, %r12
	urdmsr	%r14, %rax
	urdmsr	%r24, %rax
	urdmsr	%rdx, %r12
	urdmsr	%rdx, %r22
	urdmsr	%rdx, %rax
	urdmsr	$51515151, %r12
	urdmsr	$51515151, %rax
	urdmsr	$0x7f, %r12
	urdmsr	$0x7fff, %r12
	urdmsr	$0x80000000, %r12
	urdmsr	$0x80000000, %r22
	uwrmsr	%r12, %r14
	uwrmsr	%rax, %r14
	uwrmsr	%rax, %r24
	uwrmsr	%r12, %rdx
	uwrmsr	%r22, %rdx
	uwrmsr	%rax, %rdx
	uwrmsr	%r12, $51515151
	uwrmsr	%rax, $51515151
	uwrmsr	%r12, $0x7f
	uwrmsr	%r12, $0x7fff
	uwrmsr	%r12, $0x80000000
	uwrmsr	%r22, $0x80000000

	.intel_syntax noprefix
	urdmsr	r12, r14
	urdmsr	rax, r14
	urdmsr	r12, rdx
	urdmsr	rax, rdx
	urdmsr	r12, 51515151
	urdmsr	rax, 51515151
	urdmsr	r12, 0x7f
	urdmsr	r12, 0x7fff
	urdmsr	r12, 0x80000000
	uwrmsr	r14, r12
	uwrmsr	r14, rax
	uwrmsr	rdx, r12
	uwrmsr	rdx, rax
	uwrmsr	51515151, r12
	uwrmsr	51515151, rax
	uwrmsr	0x7f, r12
	uwrmsr	0x7fff, r12
	uwrmsr	0x80000000, r12
