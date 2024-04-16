# Check 32bit CLFLUSHOPT instructions

	.text
_start:

	clflushopt	(%ecx)	 # CLFLUSHOPT
	clflushopt	-123456(%esp,%esi,8)	 # CLFLUSHOPT

	.intel_syntax noprefix
	clflushopt	BYTE PTR [ecx]	 # CLFLUSHOPT
	clflushopt	BYTE PTR [esp+esi*8-123456]	 # CLFLUSHOPT
