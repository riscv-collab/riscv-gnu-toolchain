	.arch generic32
	.arch .avx10.1

	.include "avx512bitalg.s"

	.att_syntax prefix
	.include "avx512cd.s"

	.att_syntax prefix
	.include "avx512ifma.s"

	.att_syntax prefix
	.include "avx512vbmi.s"

	.att_syntax prefix
	.include "avx512vbmi2.s"

	.att_syntax prefix
	.include "avx512vnni.s"

	.att_syntax prefix
	.include "avx512_bf16.s"

	.att_syntax prefix
	.include "avx512_vpopcntdq.s"
