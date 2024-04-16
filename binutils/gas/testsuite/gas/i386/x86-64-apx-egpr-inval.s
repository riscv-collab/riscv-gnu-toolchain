# Check illegal 64bit APX_F instructions
	.text
	.arch .noapx_f
	test    $0x7, %r17d
	.arch .apx_f
	test    $0x7, %r17d
	xsave (%r16, %rbx)
	xsave64 (%r16, %r31)
	xrstor (%r16, %rbx)
	xrstor64 (%r16, %rbx)
	xsaves (%rbx, %r16)
	xsaves64 (%r16, %rbx)
	xrstors (%rbx, %r31)
	xrstors64 (%r16, %rbx)
	xsaveopt (%r16, %rbx)
	xsaveopt64 (%r16, %r31)
	xsavec (%r16, %rbx)
	xsavec64 (%r16, %r31)
#SSE
	blendpd $100,(%r18),%xmm6
	blendps $100,(%r18),%xmm6
	blendvpd %xmm0,(%r19),%xmm6
	blendvpd (%r19),%xmm6
	blendvps %xmm0,(%r19),%xmm6
	blendvps (%r19),%xmm6
	dppd $100,(%r20),%xmm6
	dpps $100,(%r20),%xmm6
	extractps $100,%xmm4,%r21
	extractps $100,%xmm4,(%r21)
	insertps $100,(%r21),%xmm6
	movntdqa (%r21),%xmm4
	mpsadbw $100,(%r21),%xmm6
	pabsb (%r17),%xmm0
	pabsd (%r17),%xmm0
	pabsw (%r17),%xmm0
	packusdw (%r21),%xmm6
	palignr $100,(%r17),%xmm6
	pblendvb %xmm0,(%r22),%xmm6
	pblendvb (%r22),%xmm6
	pblendw $100,(%r22),%xmm6
	pcmpeqq (%r22),%xmm6
	pcmpestri $100,(%r25),%xmm6
	pcmpestrm $100,(%r25),%xmm6
	pcmpgtq (%r25),%xmm4
	pcmpistri $100,(%r25),%xmm6
	pcmpistrm $100,(%r25),%xmm6
	pextrb $100,%xmm4,%r22
	pextrb $100,%xmm4,(%r22)
	pextrd $100,%xmm4,(%r22)
	pextrq $100,%xmm4,(%r22)
	pextrw $100,%xmm4,(%r22)
	phaddd  (%r17),%xmm0
	phaddsw (%r17),%xmm0
	phaddw  (%r17),%xmm0
	phminposuw (%r23),%xmm4
	phsubw (%r17),%xmm0
	pinsrb $100,%r23,%xmm4
	pinsrb $100,(%r23),%xmm4
	pinsrd $100, %r23d, %xmm4
	pinsrd $100,(%r23),%xmm4
	pinsrq $100, %r24, %xmm4
	pinsrq $100,(%r24),%xmm4
	pmaddubsw (%r17),%xmm0
	pmaxsb (%r24),%xmm6
	pmaxsd (%r24),%xmm6
	pmaxud (%r24),%xmm6
	pmaxuw (%r24),%xmm6
	pminsb (%r24),%xmm6
	pminsd (%r24),%xmm6
	pminud (%r24),%xmm6
	pminuw (%r24),%xmm6
	pmovsxbd (%r24),%xmm4
	pmovsxbq (%r24),%xmm4
	pmovsxbw (%r24),%xmm4
	pmovsxbw (%r24),%xmm4
	pmovsxdq (%r24),%xmm4
	pmovsxwd (%r24),%xmm4
	pmovsxwq (%r24),%xmm4
	pmovzxbd (%r24),%xmm4
	pmovzxbq (%r24),%xmm4
	pmovzxdq (%r24),%xmm4
	pmovzxwd (%r24),%xmm4
	pmovzxwq (%r24),%xmm4
	pmuldq (%r24),%xmm4
	pmulhrsw (%r17),%xmm0
	pmulld (%r24),%xmm4
	pshufb (%r17),%xmm0
	psignb (%r17),%xmm0
	psignd (%r17),%xmm0
	psignw (%r17),%xmm0
	roundpd $100,(%r24),%xmm6
	roundps $100,(%r24),%xmm6
	roundsd $100,(%r24),%xmm6
	roundss $100,(%r24),%xmm6
#AES
	aesdec (%r26),%xmm6
	aesdeclast (%r26),%xmm6
	aesenc (%r26),%xmm6
	aesenclast (%r26),%xmm6
	aesimc (%r26),%xmm6
	aeskeygenassist $100,(%r26),%xmm6
	pclmulhqhqdq (%r26),%xmm6
	pclmulhqlqdq (%r26),%xmm6
	pclmullqhqdq (%r26),%xmm6
	pclmullqlqdq (%r26),%xmm6
	pclmulqdq $100,(%r26),%xmm6
#GFNI
	gf2p8affineinvqb $100,(%r26),%xmm6
	gf2p8affineqb $100,(%r26),%xmm6
	gf2p8mulb (%r26),%xmm6
#VEX without evex
	vaesimc (%r27), %xmm3
	vaeskeygenassist $7,(%r27),%xmm3
	vblendpd $7,(%r27),%xmm6,%xmm2
	vblendpd $7,(%r27),%ymm6,%ymm2
	vblendps $7,(%r27),%xmm6,%xmm2
	vblendps $7,(%r27),%ymm6,%ymm2
	vblendvpd %xmm4,(%r27),%xmm2,%xmm7
	vblendvpd %ymm4,(%r27),%ymm2,%ymm7
	vblendvps %xmm4,(%r27),%xmm2,%xmm7
	vblendvps %ymm4,(%r27),%ymm2,%ymm7
	vdppd $7,(%r27),%xmm6,%xmm2
	vdpps $7,(%r27),%xmm6,%xmm2
	vdpps $7,(%r27),%ymm6,%ymm2
	vhaddpd (%r27),%xmm6,%xmm5
	vhaddpd (%r27),%ymm6,%ymm5
	vhsubps (%r27),%xmm6,%xmm5
	vhsubps (%r27),%ymm6,%ymm5
	vlddqu (%r27),%xmm4
	vlddqu (%r27),%ymm4
	vldmxcsr (%r27)
	vmaskmovpd %xmm4,%xmm6,(%r27)
	vmaskmovpd %ymm4,%ymm6,(%r27)
	vmaskmovpd (%r27),%xmm4,%xmm6
	vmaskmovpd (%r27),%ymm4,%ymm6
	vmaskmovps %xmm4,%xmm6,(%r27)
	vmaskmovps %ymm4,%ymm6,(%r27)
	vmaskmovps (%r27),%xmm4,%xmm6
	vmaskmovps (%r27),%ymm4,%ymm6
	vmovmskpd %xmm4,%r27d
	vmovmskpd %xmm8,%r27d
	vmovmskps %xmm4,%r27d
	vmovmskps %ymm8,%r27d
	vpblendd $7,(%r27),%xmm6,%xmm2
	vpblendd $7,(%r27),%ymm6,%ymm2
	vpblendvb %xmm4,(%r27),%xmm2,%xmm7
	vpblendvb %ymm4,(%r27),%ymm2,%ymm7
	vpblendw $7,(%r27),%xmm6,%xmm2
	vpblendw $7,(%r27),%ymm6,%ymm2
	vpcmpeqb (%r26),%ymm6,%ymm2
	vpcmpeqd (%r26),%ymm6,%ymm2
	vpcmpeqq (%r16),%ymm6,%ymm2
	vpcmpeqw (%r16),%ymm6,%ymm2
	vpcmpestri $7,(%r27),%xmm6
	vpcmpestrm $7,(%r27),%xmm6
	vpcmpgtb (%r26),%ymm6,%ymm2
	vpcmpgtd (%r26),%ymm6,%ymm2
	vpcmpgtq (%r16),%ymm6,%ymm2
	vpcmpgtw (%r16),%ymm6,%ymm2
	vpcmpistri $100,(%r25),%xmm6
	vpcmpistrm $100,(%r25),%xmm6
	vperm2f128 $7,(%r27),%ymm6,%ymm2
	vperm2i128 $7,(%r27),%ymm6,%ymm2
	vphaddd (%r27),%xmm6,%xmm7
	vphaddd (%r27),%ymm6,%ymm7
	vphaddsw (%r27),%xmm6,%xmm7
	vphaddsw (%r27),%ymm6,%ymm7
	vphaddw (%r27),%xmm6,%xmm7
	vphaddw (%r27),%ymm6,%ymm7
	vphminposuw (%r27),%xmm6
	vphsubd (%r27),%xmm6,%xmm7
	vphsubd (%r27),%ymm6,%ymm7
	vphsubsw (%r27),%xmm6,%xmm7
	vphsubsw (%r27),%ymm6,%ymm7
	vphsubw (%r27),%xmm6,%xmm7
	vphsubw (%r27),%ymm6,%ymm7
	vpmaskmovd %xmm4,%xmm6,(%r27)
	vpmaskmovd %ymm4,%ymm6,(%r27)
	vpmaskmovd (%r27),%xmm4,%xmm6
	vpmaskmovd (%r27),%ymm4,%ymm6
	vpmaskmovq %xmm4,%xmm6,(%r27)
	vpmaskmovq %ymm4,%ymm6,(%r27)
	vpmaskmovq (%r27),%xmm4,%xmm6
	vpmaskmovq (%r27),%ymm4,%ymm6
	vpmovmskb %xmm4,%r27
	vpmovmskb %ymm4,%r27d
	vpsignb (%r27),%xmm6,%xmm7
	vpsignb (%r27),%xmm6,%xmm7
	vpsignd (%r27),%xmm6,%xmm7
	vpsignd (%r27),%xmm6,%xmm7
	vpsignw (%r27),%xmm6,%xmm7
	vpsignw (%r27),%xmm6,%xmm7
	vptest (%r27),%ymm6
	vptest (%r27),%xmm6
	vrcpps (%r27),%xmm6
	vrcpps (%r27),%ymm6
	vrcpss (%r27),%xmm6,%xmm6
	vroundpd $0x11,(%r24),%xmm6
	vroundps $0x22,(%r24),%xmm6
	vroundsd $0x33,(%r24),%xmm6,%xmm3
	vroundss $0x44,(%r24),%xmm6,%xmm3
	vrsqrtps (%r27),%xmm6
	vrsqrtps (%r27),%ymm6
	vrsqrtss (%r27),%xmm6,%xmm6
	vstmxcsr (%r27)
	vtestpd (%r27),%xmm6
	vtestpd (%r27),%ymm6
	vtestps (%r27),%xmm6
	vtestps (%r27),%ymm6
