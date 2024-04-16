	.arch generic32
	.arch .avx10.1/256

	.include "avx512bitalg_vl.s"

	.att_syntax prefix
	.include "avx512cd_vl.s"

	.att_syntax prefix
	.include "avx512ifma_vl.s"

	.att_syntax prefix
	.include "avx512vbmi_vl.s"

	.att_syntax prefix
	.include "avx512vbmi2_vl.s"

	.att_syntax prefix
	.include "avx512vnni_vl.s"

	.att_syntax prefix
	.include "avx512_bf16_vl.s"

	.att_syntax prefix
	.include "avx512_vpopcntdq_vl.s"
