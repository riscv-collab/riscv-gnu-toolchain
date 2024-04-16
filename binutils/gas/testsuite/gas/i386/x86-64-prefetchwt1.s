# Check 64bit PREFETCHWT1 instructions

	.text
_start:

	prefetchwt1	(%rcx)	 # AVX512PF
	prefetchwt1	0x123(%rax,%r14,8)	 # AVX512PF

	.intel_syntax noprefix

	prefetchwt1	BYTE PTR [rcx]	 # AVX512PF
	prefetchwt1	BYTE PTR [rax+r14*8+0x1234]	 # AVX512PF
