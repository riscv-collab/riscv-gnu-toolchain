# Check 64bit APX_F EVEX-Promoted instructions.

	.text
_start:
	aadd	%r25d,0x123(%r31,%rax,4)
	aadd	%r31,0x123(%r31,%rax,4)
	aand	%r25d,0x123(%r31,%rax,4)
	aand	%r31,0x123(%r31,%rax,4)
	aesdec128kl	0x123(%r31,%rax,4),%xmm22
	aesdec256kl	0x123(%r31,%rax,4),%xmm22
	aesdecwide128kl	0x123(%r31,%rax,4)
	aesdecwide256kl	0x123(%r31,%rax,4)
	aesenc128kl	0x123(%r31,%rax,4),%xmm22
	aesenc256kl	0x123(%r31,%rax,4),%xmm22
	aesencwide128kl	0x123(%r31,%rax,4)
	aesencwide256kl	0x123(%r31,%rax,4)
	aor	%r25d,0x123(%r31,%rax,4)
	aor	%r31,0x123(%r31,%rax,4)
	axor	%r25d,0x123(%r31,%rax,4)
	axor	%r31,0x123(%r31,%rax,4)
	bextr	%r25d,%edx,%r10d
	bextr	%r25d,0x123(%r31,%rax,4),%edx
	bextr	%r31,%r15,%r11
	bextr	%r31,0x123(%r31,%rax,4),%r15
	blsi	%r25d,%edx
	blsi	%r31,%r15
	blsi	0x123(%r31,%rax,4),%r25d
	blsi	0x123(%r31,%rax,4),%r31
	blsmsk	%r25d,%edx
	blsmsk	%r31,%r15
	blsmsk	0x123(%r31,%rax,4),%r25d
	blsmsk	0x123(%r31,%rax,4),%r31
	blsr	%r25d,%edx
	blsr	%r31,%r15
	blsr	0x123(%r31,%rax,4),%r25d
	blsr	0x123(%r31,%rax,4),%r31
	bzhi	%r25d,%edx,%r10d
	bzhi	%r25d,0x123(%r31,%rax,4),%edx
	bzhi	%r31,%r15,%r11
	bzhi	%r31,0x123(%r31,%rax,4),%r15
	cmpbexadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpbexadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpbxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpbxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmplxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmplxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnbexadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnbexadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnbxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnbxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnlexadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnlexadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnlxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnlxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnoxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnoxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnpxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnpxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnsxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnsxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpnzxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpnzxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpoxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpoxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmppxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmppxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpsxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpsxadd	%r31,%r15,0x123(%r31,%rax,4)
	cmpzxadd	%r25d,%edx,0x123(%r31,%rax,4)
	cmpzxadd	%r31,%r15,0x123(%r31,%rax,4)
	crc32q	%r31, %r22
	crc32q	(%r31), %r22
	crc32b	%r19b, %r17
	crc32b	%r19b, %r21d
	crc32b	(%r19),%ebx
	crc32l	%r31d, %r23d
	crc32l	(%r31), %r23d
	crc32w	%r31w, %r21d
	crc32w	(%r31),%r21d
	crc32	%rax, %r18
	encodekey128	%r25d,%edx
	encodekey256	%r25d,%edx
	enqcmd	0x123(%r31d,%eax,4),%r25d
	enqcmd	0x123(%r31,%rax,4),%r31
	enqcmds	0x123(%r31d,%eax,4),%r25d
	enqcmds	0x123(%r31,%rax,4),%r31
	invept	0x123(%r31,%rax,4),%r31
	invpcid	0x123(%r31,%rax,4),%r31
	invvpid	0x123(%r31,%rax,4),%r31
	kmovb	%k5,%r25d
	kmovb	%k5,0x123(%r31,%rax,4)
	kmovb	%r25d,%k5
	kmovb	0x123(%r31,%rax,4),%k5
	kmovd	%k5,%r25d
	kmovd	%k5,0x123(%r31,%rax,4)
	kmovd	%r25d,%k5
	kmovd	0x123(%r31,%rax,4),%k5
	kmovq	%k5,%r31
	kmovq	%k5,0x123(%r31,%rax,4)
	kmovq	%r31,%k5
	kmovq	0x123(%r31,%rax,4),%k5
	kmovw	%k5,%r25d
	kmovw	%k5,0x123(%r31,%rax,4)
	kmovw	%r25d,%k5
	kmovw	0x123(%r31,%rax,4),%k5
	ldtilecfg	0x123(%r31,%rax,4)
	movbe	%r18w,%ax
	movbe	%r18w,0x123(%r16,%rax,4)
	movbe	%r18w,0x123(%r31,%rax,4)
	movbe	%r25d,%edx
	movbe	%r25d,0x123(%r16,%rax,4)
	movbe	%r31,%r15
	movbe	%r31,0x123(%r16,%rax,4)
	movbe	%r31,0x123(%r31,%rax,4)
	movbe	0x123(%r16,%rax,4),%r31
	movbe	0x123(%r31,%rax,4),%r18w
	movbe	0x123(%r31,%rax,4),%r25d
	movdir64b	0x123(%r31d,%eax,4),%r25d
	movdir64b	0x123(%r31,%rax,4),%r31
	movdiri	%r25d,0x123(%r31,%rax,4)
	movdiri	%r31,0x123(%r31,%rax,4)
	pdep	%r25d,%edx,%r10d
	pdep	%r31,%r15,%r11
	pdep	0x123(%r31,%rax,4),%r25d,%edx
	pdep	0x123(%r31,%rax,4),%r31,%r15
	pext	%r25d,%edx,%r10d
	pext	%r31,%r15,%r11
	pext	0x123(%r31,%rax,4),%r25d,%edx
	pext	0x123(%r31,%rax,4),%r31,%r15
	sha1msg1	%xmm23,%xmm22
	sha1msg1	0x123(%r31,%rax,4),%xmm22
	sha1msg2	%xmm23,%xmm22
	sha1msg2	0x123(%r31,%rax,4),%xmm22
	sha1nexte	%xmm23,%xmm22
	sha1nexte	0x123(%r31,%rax,4),%xmm22
	sha1rnds4	$0x7b,%xmm23,%xmm22
	sha1rnds4	$0x7b,0x123(%r31,%rax,4),%xmm22
	sha256msg1	%xmm23,%xmm22
	sha256msg1	0x123(%r31,%rax,4),%xmm22
	sha256msg2	%xmm23,%xmm22
	sha256msg2	0x123(%r31,%rax,4),%xmm22
	sha256rnds2	0x123(%r31,%rax,4),%xmm12
	shlx	%r25d,%edx,%r10d
	shlx	%r25d,0x123(%r31,%rax,4),%edx
	shlx	%r31,%r15,%r11
	shlx	%r31,0x123(%r31,%rax,4),%r15
	shrx	%r25d,%edx,%r10d
	shrx	%r25d,0x123(%r31,%rax,4),%edx
	shrx	%r31,%r15,%r11
	shrx	%r31,0x123(%r31,%rax,4),%r15
	sttilecfg	0x123(%r31,%rax,4)
	tileloadd	0x123(%r31,%rax,4),%tmm6
	tileloaddt1	0x123(%r31,%rax,4),%tmm6
	tilestored	%tmm6,0x123(%r31,%rax,4)
	vroundpd $1,(%r24),%xmm6
	vroundps $2,(%r24),%xmm6
	vroundsd $3,(%r24),%xmm6,%xmm3
	vroundss $4,(%r24),%xmm6,%xmm3
	wrssd	%r25d,0x123(%r31,%rax,4)
	wrssq	%r31,0x123(%r31,%rax,4)
	wrussd	%r25d,0x123(%r31,%rax,4)
	wrussq	%r31,0x123(%r31,%rax,4)

	.intel_syntax noprefix
	aadd	[r31+rax*4+0x123],r25d
	aadd	[r31+rax*4+0x123],r31
	aand	[r31+rax*4+0x123],r25d
	aand	[r31+rax*4+0x123],r31
	aesdec128kl	xmm22,[r31+rax*4+0x123]
	aesdec256kl	xmm22,[r31+rax*4+0x123]
	aesdecwide128kl	[r31+rax*4+0x123]
	aesdecwide256kl	[r31+rax*4+0x123]
	aesenc128kl	xmm22,[r31+rax*4+0x123]
	aesenc256kl	xmm22,[r31+rax*4+0x123]
	aesencwide128kl	[r31+rax*4+0x123]
	aesencwide256kl	[r31+rax*4+0x123]
	aor	[r31+rax*4+0x123],r25d
	aor	[r31+rax*4+0x123],r31
	axor	[r31+rax*4+0x123],r25d
	axor	[r31+rax*4+0x123],r31
	bextr	r10d,edx,r25d
	bextr	edx, [r31+rax*4+0x123],r25d
	bextr	r11,r15,r31
	bextr	r15, [r31+rax*4+0x123],r31
	blsi	edx,r25d
	blsi	r15,r31
	blsi	r25d, [r31+rax*4+0x123]
	blsi	r31,  [r31+rax*4+0x123]
	blsmsk	edx,r25d
	blsmsk	r15,r31
	blsmsk	r25d, [r31+rax*4+0x123]
	blsmsk	r31,  [r31+rax*4+0x123]
	blsr	edx,r25d
	blsr	r15,r31
	blsr	r25d, [r31+rax*4+0x123]
	blsr	r31,  [r31+rax*4+0x123]
	bzhi	r10d,edx,r25d
	bzhi	edx, [r31+rax*4+0x123],r25d
	bzhi	r11,r15,r31
	bzhi	r15, [r31+rax*4+0x123],r31
	cmpbexadd	 [r31+rax*4+0x123],edx,r25d
	cmpbexadd	 [r31+rax*4+0x123],r15,r31
	cmpbxadd	 [r31+rax*4+0x123],edx,r25d
	cmpbxadd	 [r31+rax*4+0x123],r15,r31
	cmplxadd	 [r31+rax*4+0x123],edx,r25d
	cmplxadd	 [r31+rax*4+0x123],r15,r31
	cmpnbexadd	 [r31+rax*4+0x123],edx,r25d
	cmpnbexadd	 [r31+rax*4+0x123],r15,r31
	cmpnbxadd	 [r31+rax*4+0x123],edx,r25d
	cmpnbxadd	 [r31+rax*4+0x123],r15,r31
	cmpnlexadd	 [r31+rax*4+0x123],edx,r25d
	cmpnlexadd	 [r31+rax*4+0x123],r15,r31
	cmpnlxadd	 [r31+rax*4+0x123],edx,r25d
	cmpnlxadd	 [r31+rax*4+0x123],r15,r31
	cmpnoxadd	 [r31+rax*4+0x123],edx,r25d
	cmpnoxadd	 [r31+rax*4+0x123],r15,r31
	cmpnpxadd	 [r31+rax*4+0x123],edx,r25d
	cmpnpxadd	 [r31+rax*4+0x123],r15,r31
	cmpnsxadd	 [r31+rax*4+0x123],edx,r25d
	cmpnsxadd	 [r31+rax*4+0x123],r15,r31
	cmpnzxadd	 [r31+rax*4+0x123],edx,r25d
	cmpnzxadd	 [r31+rax*4+0x123],r15,r31
	cmpoxadd	 [r31+rax*4+0x123],edx,r25d
	cmpoxadd	 [r31+rax*4+0x123],r15,r31
	cmppxadd	 [r31+rax*4+0x123],edx,r25d
	cmppxadd	 [r31+rax*4+0x123],r15,r31
	cmpsxadd	 [r31+rax*4+0x123],edx,r25d
	cmpsxadd	 [r31+rax*4+0x123],r15,r31
	cmpzxadd	 [r31+rax*4+0x123],edx,r25d
	cmpzxadd	 [r31+rax*4+0x123],r15,r31
	crc32	r22,r31
	crc32	r22,QWORD PTR [r31]
	crc32	r17,r19b
	crc32	r21d,r19b
	crc32	ebx,BYTE PTR [r19]
	crc32	r23d,r31d
	crc32	r23d,DWORD PTR [r31]
	crc32	r21d,r31w
	crc32	r21d,WORD PTR [r31]
	crc32	r18,rax
	encodekey128	edx,r25d
	encodekey256	edx,r25d
	enqcmd	r25d,[r31d+eax*4+0x123]
	enqcmd	r31,[r31+rax*4+0x123]
	enqcmds	r25d,[r31d+eax*4+0x123]
	enqcmds	r31,[r31+rax*4+0x123]
	invept	r31,OWORD PTR [r31+rax*4+0x123]
	invpcid	r31,[r31+rax*4+0x123]
	invvpid	r31,OWORD PTR [r31+rax*4+0x123]
	kmovb	r25d,k5
	kmovb	BYTE PTR [r31+rax*4+0x123],k5
	kmovb	k5,r25d
	kmovb	k5,BYTE PTR [r31+rax*4+0x123]
	kmovd	r25d,k5
	kmovd	DWORD PTR [r31+rax*4+0x123],k5
	kmovd	k5,r25d
	kmovd	k5,DWORD PTR [r31+rax*4+0x123]
	kmovq	r31,k5
	kmovq	QWORD PTR [r31+rax*4+0x123],k5
	kmovq	k5,r31
	kmovq	k5,QWORD PTR [r31+rax*4+0x123]
	kmovw	r25d,k5
	kmovw	WORD PTR [r31+rax*4+0x123],k5
	kmovw	k5,r25d
	kmovw	k5,WORD PTR [r31+rax*4+0x123]
	ldtilecfg	[r31+rax*4+0x123]
	movbe	ax,r18w
	movbe	WORD PTR [r16+rax*4+0x123],r18w
	movbe	WORD PTR [r31+rax*4+0x123],r18w
	movbe	edx,r25d
	movbe	DWORD PTR [r16+rax*4+0x123],r25d
	movbe	r15,r31
	movbe	QWORD PTR [r16+rax*4+0x123],r31
	movbe	QWORD PTR [r31+rax*4+0x123],r31
	movbe	r31,QWORD PTR [r16+rax*4+0x123]
	movbe	r18w,WORD PTR [r31+rax*4+0x123]
	movbe	r25d,DWORD PTR [r31+rax*4+0x123]
	movdir64b	r25d,[r31d+eax*4+0x123]
	movdir64b	r31,[r31+rax*4+0x123]
	movdiri	DWORD PTR [r31+rax*4+0x123],r25d
	movdiri	QWORD PTR [r31+rax*4+0x123],r31
	pdep	r10d,edx,r25d
	pdep	r11,r15,r31
	pdep	edx,r25d,DWORD PTR [r31+rax*4+0x123]
	pdep	r15,r31,QWORD PTR [r31+rax*4+0x123]
	pext	r10d,edx,r25d
	pext	r11,r15,r31
	pext	edx,r25d,DWORD PTR [r31+rax*4+0x123]
	pext	r15,r31,QWORD PTR [r31+rax*4+0x123]
	sha1msg1	xmm22,xmm23
	sha1msg1	xmm22,XMMWORD PTR [r31+rax*4+0x123]
	sha1msg2	xmm22,xmm23
	sha1msg2	xmm22,XMMWORD PTR [r31+rax*4+0x123]
	sha1nexte	xmm22,xmm23
	sha1nexte	xmm22,XMMWORD PTR [r31+rax*4+0x123]
	sha1rnds4	xmm22,xmm23,0x7b
	sha1rnds4	xmm22,XMMWORD PTR [r31+rax*4+0x123],0x7b
	sha256msg1	xmm22,xmm23
	sha256msg1	xmm22,XMMWORD PTR [r31+rax*4+0x123]
	sha256msg2	xmm22,xmm23
	sha256msg2	xmm22,XMMWORD PTR [r31+rax*4+0x123]
	sha256rnds2	xmm12,XMMWORD PTR [r31+rax*4+0x123]
	shlx	r10d,edx,r25d
	shlx	edx,DWORD PTR [r31+rax*4+0x123],r25d
	shlx	r11,r15,r31
	shlx	r15,QWORD PTR [r31+rax*4+0x123],r31
	shrx	r10d,edx,r25d
	shrx	edx,DWORD PTR [r31+rax*4+0x123],r25d
	shrx	r11,r15,r31
	shrx	r15,QWORD PTR [r31+rax*4+0x123],r31
	sttilecfg	[r31+rax*4+0x123]
	tileloadd	tmm6,[r31+rax*4+0x123]
	tileloaddt1	tmm6,[r31+rax*4+0x123]
	tilestored	[r31+rax*4+0x123],tmm6
	wrssd	DWORD PTR [r31+rax*4+0x123],r25d
	wrssq	QWORD PTR [r31+rax*4+0x123],r31
	wrussd	DWORD PTR [r31+rax*4+0x123],r25d
	wrussq	QWORD PTR [r31+rax*4+0x123],r31
