# Check illegal 64bit APX EVEX promoted instructions
	.text
	.arch .nomovbe
	movbe (%r16), %r17
	movbe (%rax), %rcx
	.arch default
	.arch .noept
	invept (%r16), %r17
	invept (%rax), %rcx
	.arch default
	.arch .noavx512bw
	kmovq %k1, (%r16)
	kmovq %k1, (%r8)
	.arch default
	.arch .noavx512dq
	kmovb %k1, %r16d
	kmovb %k1, %r8d
	.arch default
	.arch .noavx512f
	kmovw %k1, %r16d
	kmovw %k1, %r8d
	.arch default
	.arch .nobmi
	andn %r16,%r15,%r11
	andn %r15,%r15,%r11
	.arch default
	.arch .nobmi2
	bzhi %r16,%r15,%r11
	bzhi %r15,%r15,%r11

	.arch default
	.arch .noapx_f
	{evex} andn %r15, %r15, %r11
	{evex} bzhi %r15, %r15, %r11
	{evex} kmovw %k1, %r8d
	{evex} kmovq %k1, %r8
	{evex} kmovb %k1, %r8d
	{evex} ldtilecfg (%r8)
	{evex} cmpexadd %rax, %rcx, (%r8)
