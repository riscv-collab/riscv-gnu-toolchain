# Check 64bit old evex instructions use gpr32 with evex prefix encoding

	.allow_index_reg
	.text
_start:
## DestMem
	 vextractf32x4	$1, %zmm0, (%r16,%r17)
## SrcMem
	 vbroadcasti32x4	(%r18,%r19), %zmm0
## DestReg
	 vextractps	$1, %xmm16, %r20d
## SrcReg
	 vcvtsi2sdq	%r21, %xmm29, %xmm30
## Broadcast
	 vfmaddsub132ph	(%r22d){1to32}, %zmm5, %zmm6
## SAE
	 vcvttss2usi	{sae}, %xmm30, %r23
## Masking
	 vaddph	0x10000000(%rbp, %r24, 8), %zmm29, %zmm30{%k7}
## Disp8memshift
	 vcomish	254(%r25), %xmm30
## For dual AVX/AVX512 templates
	 vmovq	0x8(%r26),%xmm2
