# Check 32bit CLWB instructions

	.text
_start:

	clwb	(%ecx)	 # CLWB
	clwb	-123456(%esp,%esi,8)	 # CLWB

	.intel_syntax noprefix
	clwb	BYTE PTR [ecx]	 # CLWB
	clwb	BYTE PTR [esp+esi*8-123456]	 # CLWB
