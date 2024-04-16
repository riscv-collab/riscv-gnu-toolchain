	.text
prefix:
	addr16 loop	.Lrep_ret
	addr16 jecxz	.Ldata16

	repe addss	%xmm0, %xmm0
	repne addss	%xmm0, %xmm0
	repe vaddss	%xmm0, %xmm0, %xmm0
	repne vaddss	%xmm0, %xmm0, %xmm0

.Lrep_ret:
	bnd ret
	rep ret
	bnd rep ret
	rep bnd ret

.Ldata16:
	data16 addps	%xmm0, %xmm0
	data16 addpd	%xmm0, %xmm0
	data16 vaddps	%xmm0, %xmm0, %xmm0
	data16 vaddpd	%xmm0, %xmm0, %xmm0

.Lsegment:
	ss mov		%ss:(%ebp), %eax
	ss mov		%ds:(%ebp), %eax
	ds mov		%ss:(%ebp), %eax
	ds mov		%ds:(%ebp), %eax

.L386:
	.arch i386
	xacquire lock add [esi], eax
	notrack call eax
	bnd call eax

.L286:
	.code16
	.arch i286
	gs inc word ptr [si]
	data32 nop
	addr32 nop
	{disp32} nop

	.p2align	4,0
